#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#include "pretty_effect.h"
#include <libesp/system.h>
#include <libesp/i2c.hpp>
#include <esp_log.h>

#include "nvs_flash.h"

#include "./ble.h"
#include "./serial_game.h"

/*
 This code displays some fancy graphics on the 320x240 LCD on an ESP-WROVER_KIT board.
 This example demonstrates the use of both spi_device_transmit as well as
 spi_device_queue_trans/spi_device_get_trans_result and pre-transmit callbacks.

 Some info about the ILI9341/ST7789V: It has an C/D line, which is connected to a GPIO here. It expects this
 line to be low for a command and high for data. We use a pre-transmit callback here to control that
 line: every transaction has as the user-definable argument the needed state of the D/C line and just
 before the transaction is sent, the callback will set this line to the correct state.
*/

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5


#define LED_PIN_MOSI 13
#define LED_PIN_CLK  14
#define LED_PIN_NONE -1
//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

extern "C" {
	void app_main();
}

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

typedef enum {
    LCD_TYPE_ILI = 1,
    LCD_TYPE_ST,
    LCD_TYPE_MAX,
} type_lcd_t;

//Place data into DRAM. Constant data gets placed into DROM by default, which is not accessible by DMA.
DRAM_ATTR static const lcd_init_cmd_t st_init_cmds[]={
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    {0x36, {(1<<5)|(1<<6)}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Porch Setting */
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {0xB7, {0x45}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {0xBB, {0x2B}, 1},
    /* LCM Control, XOR: RGB, MX, MH */
    {0xC0, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {0xC2, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {0xC3, {0x11}, 1},
    /* VDV Set, VDV=0 */
    {0xC4, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {0xC6, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {0xD0, {0xA4, 0xA1}, 1},
    /* Positive Voltage Gamma Control */
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
    /* Negative Voltage Gamma Control */
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},
    /* Sleep Out */
    {0x11, {0}, 0x80},
    /* Display On */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff}
};

DRAM_ATTR static const lcd_init_cmd_t ili_init_cmds[]={
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xBE}, 1},
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, RGB=1, MH=0 */
    {0x36, {0x28}, 1},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x08}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
    /* Negative gamma correction */
    {0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {0x2C, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

//Send a command to the LCD. Uses spi_device_transmit, which waits until the transfer is complete.
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

//Send data to the LCD. Uses spi_device_transmit, which waits until the transfer is complete.
void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level((gpio_num_t)PIN_NUM_DC, dc);
}

uint32_t lcd_get_id(spi_device_handle_t spi)
{
    //get_id cmd
    lcd_cmd( spi, 0x04);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8*3;
    t.flags = SPI_TRANS_USE_RXDATA;
    t.user = (void*)1;

    esp_err_t ret = spi_device_transmit(spi, &t);
    assert( ret == ESP_OK );

    return *(uint32_t*)t.rx_data;
}

//Initialize the display
void lcd_init(spi_device_handle_t spi)
{
    int cmd=0;
    const lcd_init_cmd_t* lcd_init_cmds;

    //Initialize non-SPI GPIOs
    gpio_set_direction((gpio_num_t)PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    //Reset the display
    gpio_set_level((gpio_num_t)PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level((gpio_num_t)PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

    //detect LCD type
    uint32_t lcd_id = lcd_get_id(spi);
    int lcd_detected_type = 0;
    int lcd_type;

    printf("LCD ID: %08X\n", lcd_id);
    if ( lcd_id == 0 ) {
        //zero, ili
        lcd_detected_type = LCD_TYPE_ILI;
        printf("ILI9341 detected.\n");
    } else {
        // none-zero, ST
        lcd_detected_type = LCD_TYPE_ST;
        printf("ST7789V detected.\n");
    }

#ifdef CONFIG_LCD_TYPE_AUTO
    lcd_type = lcd_detected_type;
#elif defined( CONFIG_LCD_TYPE_ST7789V )
    printf("kconfig: force CONFIG_LCD_TYPE_ST7789V.\n");
    lcd_type = LCD_TYPE_ST;
#elif defined( CONFIG_LCD_TYPE_ILI9341 )
    printf("kconfig: force CONFIG_LCD_TYPE_ILI9341.\n");
    lcd_type = LCD_TYPE_ILI;
#endif
    if ( lcd_type == LCD_TYPE_ST ) {
        printf("LCD ST7789V initialization.\n");
        lcd_init_cmds = st_init_cmds;
    } else {
        printf("LCD ILI9341 initialization.\n");
        lcd_init_cmds = ili_init_cmds;
    }

    //Send all the commands
    while (lcd_init_cmds[cmd].databytes!=0xff) {
        lcd_cmd(spi, lcd_init_cmds[cmd].cmd);
        lcd_data(spi, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes&0x1F);
        if (lcd_init_cmds[cmd].databytes&0x80) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }

    ///Enable backlight
    gpio_set_level((gpio_num_t)PIN_NUM_BCKL, 0);
}


//To send a set of lines we have to send a command, 2 data bytes, another command, 2 more data bytes and another command
//before sending the line data itself; a total of 6 transactions. (We can't put all of this in just one transaction
//because the D/C line needs to be toggled in the middle.)
//This routine queues these commands up so they get sent as quickly as possible.
static void send_lines(spi_device_handle_t spi, int ypos, uint16_t *linedata)
{
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x=0; x<6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x&1)==0) {
            //Even transfers are commands
            trans[x].length=8;
            trans[x].user=(void*)0;
        } else {
            //Odd transfers are data
            trans[x].length=8*4;
            trans[x].user=(void*)1;
        }
        trans[x].flags=SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0]=0x2A;           //Column Address Set
    trans[1].tx_data[0]=0;              //Start Col High
    trans[1].tx_data[1]=0;              //Start Col Low
    trans[1].tx_data[2]=(320)>>8;       //End Col High
    trans[1].tx_data[3]=(320)&0xff;     //End Col Low
    trans[2].tx_data[0]=0x2B;           //Page address set
    trans[3].tx_data[0]=ypos>>8;        //Start page high
    trans[3].tx_data[1]=ypos&0xff;      //start page low
    trans[3].tx_data[2]=(ypos+PARALLEL_LINES)>>8;    //end page high
    trans[3].tx_data[3]=(ypos+PARALLEL_LINES)&0xff;  //end page low
    trans[4].tx_data[0]=0x2C;           //memory write
    trans[5].tx_buffer=linedata;        //finally send the line data
    trans[5].length=320*2*8*PARALLEL_LINES;          //Data length, in bits
    trans[5].flags=0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x=0; x<6; x++) {
        ret=spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret==ESP_OK);
    }

    //When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
    //mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
    //finish because we may as well spend the time calculating the next line. When that is done, we can call
    //send_line_finish, which will wait for the transfers to be done and check their status.
}


static void send_line_finish(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x=0; x<6; x++) {
        ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret==ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}


//Simple routine to generate some patterns and send them to the LCD. Don't expect anything too
//impressive. Because the SPI driver handles transactions in the background, we can calculate the next line
//while the previous one is being sent.
static void display_pretty_colors(spi_device_handle_t spi)
{
    uint16_t *lines[2];
    //Allocate memory for the pixel buffers
    for (int i=0; i<2; i++) {
        lines[i]=(uint16_t*)heap_caps_malloc(320*PARALLEL_LINES*sizeof(uint16_t), MALLOC_CAP_DMA);
        assert(lines[i]!=NULL);
    }
    int frame=0;
    //Indexes of the line currently being sent to the LCD and the line we're calculating.
    int sending_line=-1;
    int calc_line=0;

    while(1) {
        frame++;
        for (int y=0; y<240; y+=PARALLEL_LINES) {
            //Calculate a line.
            pretty_effect_calc_lines(lines[calc_line], y, frame, PARALLEL_LINES);
            //Finish up the sending process of the previous line, if any
            if (sending_line!=-1) send_line_finish(spi);
            //Swap sending_line and calc_line
            sending_line=calc_line;
            calc_line=(calc_line==1)?0:1;
            //Send the line we currently calculated.
            send_lines(spi, y, lines[sending_line]);
            //The line set is queued up for sending now; the actual sending happens in the
            //background. We can go on to calculate the next line set as long as we do not
            //touch line[sending_line]; the SPI sending process is still reading from that.
        }
    }
}


//////////////////////////////////////////////
// LEDS

class ErrorType {
public:
	ErrorType(esp_err_t et) : ErrType(et) {}
	bool ok() {return ErrType==ESP_OK;}
	const char *toString() {return esp_err_to_name(ErrType);}
private:
	esp_err_t ErrType;
};

class Wiring {
public:
	Wiring(){}
	bool init() {
		return onInit();
	}
	ErrorType shutdown() {
		return onShutdown();
	}
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in)=0;
	virtual ErrorType send(uint8_t *p, uint16_t len) =0;
	virtual ErrorType onShutdown()=0;
	virtual ~Wiring() {}
protected:
	virtual bool onInit()=0;
};

class ESP32SPIWiring : public Wiring {
public:
	static ESP32SPIWiring create(spi_host_device_t shd, int miso, int mosi, int clk, int cs, int tb, int dma) {
		ESP32SPIWiring esp32(shd,miso,mosi,clk,cs, tb,dma);
		return esp32;
	}
public:
	virtual ErrorType sendAndReceive(uint8_t out, uint8_t &in) {
    	spi_transaction_t t;
    	memset(&t, 0, sizeof(t));   //Zero out the transaction
    	t.length=8;                 //Len is in bytes, transaction length is in bits.
		t.tx_data[0]=out;             //Data
		t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
		t.user=(void*)1;              
    		esp_err_t r = spi_device_transmit(spi, &t);  //Transmit!
		in = t.rx_data[0];
		return r;
	}
	virtual ErrorType send(uint8_t *p, uint16_t len) {
    		if (len==0) return ESP_OK;       //no need to send anything
    		spi_transaction_t t;
    		memset(&t, 0, sizeof(t));   //Zero out the transaction
    		t.length=len*8;                 //Len is in bytes, transaction length is in bits.
		t.tx_buffer=p;             //Data
		t.user=(void*)2;              
    		esp_err_t r = spi_device_transmit(spi, &t);  //Transmit!
		return r;
	}
	virtual ~ESP32SPIWiring() {
		shutdown();
	}
public:
	ErrorType onShutdown() {
		return spi_bus_free(SpiHD);
	}
protected:
	ESP32SPIWiring(spi_host_device_t spihd, int miso, int mosi, int clk, int cs, int bufSize, int dmachannel) 
		: SpiHD(spihd), PinMiso(miso), PinMosi(mosi), PinCLK(clk), PinCS(cs), TransferBufferSize(bufSize), 
		DMAChannel(dmachannel) { }
	virtual bool onInit() {
		esp_err_t ret;

		spi_bus_config_t buscfg;
		buscfg.miso_io_num=PinMiso;
		buscfg.mosi_io_num=PinMosi;
		buscfg.sclk_io_num=PinCLK;
		buscfg.quadwp_io_num=-1;
		buscfg.quadhd_io_num=-1;
		buscfg.max_transfer_sz=TransferBufferSize;
		buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
		buscfg.intr_flags = 0;

		//Initialize the SPI bus
		ret=spi_bus_initialize(SpiHD, &buscfg, DMAChannel);
		ESP_ERROR_CHECK(ret);

		spi_device_interface_config_t devcfg;
		devcfg.clock_speed_hz=1*1000*1000;         //Clock out at 1 MHz
		devcfg.mode=0;                             //SPI mode 0
		devcfg.spics_io_num=PinCS;               	//CS pin
		devcfg.queue_size=3;                       //We want to be able to queue 3 transactions at a time
		devcfg.duty_cycle_pos = 0;
		devcfg.cs_ena_pretrans = 0;
		devcfg.cs_ena_posttrans = 0; 
		devcfg.input_delay_ns = 0;
		devcfg.flags = 0;
		devcfg.pre_cb = nullptr;
		devcfg.post_cb = nullptr;

		//Attach the LED to the SPI bus
		ret=spi_bus_add_device(SpiHD, &devcfg, &spi);
		ESP_ERROR_CHECK(ret);
	 	return ESP_OK==ret;
	}
private:
	spi_host_device_t SpiHD;
	int PinMiso;
	int PinMosi;
	int PinCLK;
	int PinCS;
	int TransferBufferSize;
	int DMAChannel;
	spi_device_handle_t spi;
};

/*
	Brightness is a percentage
*/
class RGB {
public:
	static const RGB WHITE;
	static const RGB BLUE;
	static const RGB GREEN;
	static const RGB RED;
public:
	RGB() : B(0), G(0), R(0), Brightness(100) {}
	RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) : B(b), G(g), R(r), Brightness(brightness) {}
	RGB(const RGB &r) : B(r.B), G(r.G), R(r.R), Brightness(r.Brightness) {}
	uint8_t getBlue()  {return B;}
	uint8_t getRed()   {return R;}
	uint8_t getGreen() {return G;}
	uint8_t getBrightness() {return Brightness;}
	void setBlue(uint8_t v) 	{B=v;}
	void setRed(uint8_t v)  	{R=v;}
	void setGreen(uint8_t v) 	{G=v;}
	void setBrightness(uint8_t v)	{Brightness=((v>100)?100:v);}
private:
	uint8_t B;
	uint8_t G;
	uint8_t R;
	uint8_t Brightness;
} __attribute__((packed));;

const RGB RGB::WHITE(255,255,255,100);
const RGB RGB::BLUE(0,0,255,100);
const RGB RGB::GREEN(0,255,0,100);
const RGB RGB::RED(255,0,0,100);

class APA102c {
public:
	//0xE0 because high 3 bits are always on
	static const uint8_t BRIGHTNESS_START_BITS = 0xE0;
	static const uint8_t MAX_BRIGHTNESS			 = 31;
	static const char *LOG;
public:
	APA102c(Wiring *spiI) : SPIInterface(spiI), BufferSize(0), LedBuffer1(0) {}
	
	void init(uint16_t nleds, RGB *ledBuf) {
		delete [] LedBuffer1;
		BufferSize = (nleds*4)+8;
		LedBuffer1 = new char [BufferSize];
		int bufOff = 0;
		LedBuffer1[bufOff]   = 0x0;
		LedBuffer1[++bufOff] = 0x0;
		LedBuffer1[++bufOff] = 0x0;
		LedBuffer1[++bufOff] = 0x0;
		for(int l=0;l<nleds;++l) {
			uint8_t bright = ledBuf[l].getBrightness();
			bright = (uint8_t)(((float)bright/100.0f)*MAX_BRIGHTNESS);
			LedBuffer1[++bufOff] = BRIGHTNESS_START_BITS|bright;
			LedBuffer1[++bufOff] = ledBuf[l].getBlue();
			LedBuffer1[++bufOff] = ledBuf[l].getGreen();
			LedBuffer1[++bufOff] = ledBuf[l].getRed();
		}
		LedBuffer1[++bufOff] = 0xFF;
		LedBuffer1[++bufOff] = 0xFF;
		LedBuffer1[++bufOff] = 0xFF;
		LedBuffer1[++bufOff] = 0xFF;
	}
	void send() {
		if(BufferSize>0) {
			ESP_LOGI(APA102c::LOG,"sending %d leds r[0]:%d, g[0]:%d, b[0]:%d, B[0]:%d\n", (BufferSize/4)-8,
				LedBuffer1[3], LedBuffer1[2], LedBuffer1[1], LedBuffer1[0]);
			ESP_LOG_BUFFER_HEX(APA102c::LOG, LedBuffer1, BufferSize);
			SPIInterface->send((uint8_t*)LedBuffer1,BufferSize);
		}
	}
private:
	Wiring *SPIInterface;
	uint16_t BufferSize;
	char *LedBuffer1;
};

const char *APA102c::LOG = "APA102c";

BluetoothTask BTTask("BluetoothTask");
SerialGameTask GameTask("SerialGameTask");

void app_main() {
	esp_err_t ret;
	
	// initialize NVS
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase()); // TODO: do we actually want this?
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	//ESP32_I2CMaster I2cDisplay(GPIO_NUM_27,GPIO_NUM_25,1000000, I2C_NUM_0, 0, 32);
	//ESP32_I2CMaster I2cDisplay(GPIO_NUM_19,GPIO_NUM_18,1000000, I2C_NUM_0, 0, 32);
	ESP32_I2CMaster::doIt();
	//I2cDisplay.init();
	//I2cDisplay.scan();
	BTTask.init();
	BTTask.start();

	GameTask.init();
	GameTask.start();
	BTTask.setGameTaskQueue(GameTask.getQueueHandle());

	libesp::System::get().logSystemInfo();	
}

#if 0
void app_main() {
	esp_err_t ret;
	
	spi_device_handle_t spi;
	spi_bus_config_t buscfg;
	memset(&buscfg,0,sizeof(buscfg));
	buscfg.miso_io_num=PIN_NUM_MISO;
        buscfg.mosi_io_num=PIN_NUM_MOSI;
        buscfg.sclk_io_num=PIN_NUM_CLK;
        buscfg.quadwp_io_num=-1;
        buscfg.quadhd_io_num=-1;
	buscfg.max_transfer_sz=PARALLEL_LINES*320*2+8;

   spi_device_interface_config_t devcfg;
	memset(&devcfg,0,sizeof(devcfg));
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz=26*1000*1000,           //Clock out at 26 MHz
#else
        devcfg.clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
#endif
        devcfg.mode=0,                                //SPI mode 0
        devcfg.spics_io_num=PIN_NUM_CS,               //CS pin
        devcfg.queue_size=7,                          //We want to be able to queue 7 transactions at a time
        devcfg.pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line

	//Initialize the SPI bus
	ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
	ESP_ERROR_CHECK(ret);
	//Attach the LCD to the SPI bus
	ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);
	//Initialize the LCD
	lcd_init(spi);
	
	//Policy,
	const int NUM_LEDS = 100;
	ESP32SPIWiring espSPI = ESP32SPIWiring::create(VSPI_HOST,LED_PIN_NONE,LED_PIN_MOSI,
																	LED_PIN_CLK,LED_PIN_NONE, 1024, 2);
	espSPI.init();
	APA102c apa102c(&espSPI);
	RGB ledBuf[NUM_LEDS] = {RGB::BLUE};
	for(int i=0;i<NUM_LEDS;i++) {
		ledBuf[i] = RGB::BLUE;
	}
	apa102c.init(NUM_LEDS,&ledBuf[0]);
	apa102c.send();
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	for(int i=0;i<NUM_LEDS;i++) {
		ledBuf[i] = RGB::GREEN;
	}
	apa102c.init(NUM_LEDS,&ledBuf[0]);
	apa102c.send();
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	for(int kk=0;kk<101;kk+=5) {
		for(int i=0;i<NUM_LEDS;i++) {
			ledBuf[i].setBrightness(kk);
		}
		apa102c.init(NUM_LEDS,&ledBuf[0]);
		apa102c.send();
		vTaskDelay(300 / portTICK_PERIOD_MS);
	}
	
	//Initialize the effect displayed
	ret=pretty_effect_init();
	ESP_ERROR_CHECK(ret);

	//Go do nice stuff.
	display_pretty_colors(spi);
}
#endif
