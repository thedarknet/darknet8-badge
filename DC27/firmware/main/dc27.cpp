#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#include <libesp/system.h>
#include <libesp/i2c.hpp>
#include <esp_log.h>
#include <libesp/spibus.h>
#include <libesp/device/touch/XPT2046.h>
#include <libesp/device/display/frame_buffer.h>
#include <libesp/device/display/display_device.h>
#include <libesp/device/display/fonts.h>

#include "nvs_flash.h"

#include "./ble.h"
#include "./game_master.h"
//#include "./ota.h"

// XXX: Games get included here and installed into the game master
#include "./exploitable.h"

#define PIN_NUM_TOUCH_MISO 35
#define PIN_NUM_TOUCH_MOSI 33
#define PIN_NUM_TOUCH_CLK  26
#define PIN_NUM_TOUCH_CS  GPIO_NUM_27
#define PIN_NUM_TOUCH_IRQ GPIO_NUM_32

#define PIN_NUM_DISPLAY_MISO GPIO_NUM_19
#define PIN_NUM_DISPLAY_MOSI GPIO_NUM_23
#define PIN_NUM_DISPLAY_CLK  GPIO_NUM_18
#define PIN_NUM_DISPLAY_CS  GPIO_NUM_17
#define PIN_NUM_DISPLAY_DATA_CMD GPIO_NUM_4

#define NOPIN ((gpio_num_t)-1) //for some reason GPIO_NUM_NC won't work
#define PIN_NUM_DISPLAY_BACKLIGHT NOPIN
#define PIN_NUM_DISPLAY_RESET NOPIN

#define I2C_SCL GPIO_NUM_22
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL2 GPIO_NUM_2
#define I2C_SDA2 GPIO_NUM_16

extern "C" {
	void app_main();
}

BluetoothTask BTTask("BluetoothTask");
GameTask GameTask("GameTask");
ExploitableGameTask ExploitTask("ExploitTask");
//OTATask OTATask("OTATask");
libesp::XPT2046 TouchTask(4,25,PIN_NUM_TOUCH_IRQ);

static const uint16_t DISPLAY_HEIGHT		= 240;
static const uint16_t DISPLAY_WIDTH			= 320;
static const uint16_t FRAME_BUFFER_HEIGHT	= 120;
static const uint16_t FRAME_BUFFER_WIDTH	= 160;
#define START_ROT libesp::DisplayILI9341::LANDSCAPE_TOP_LEFT
static const uint16_t PARALLEL_LINES = 10;

libesp::DisplayILI9341 Display(DISPLAY_WIDTH,DISPLAY_HEIGHT,START_ROT, NOPIN, NOPIN);

uint16_t BackBuffer[FRAME_BUFFER_WIDTH*FRAME_BUFFER_HEIGHT] = {0};
uint16_t ParallelLinesBuffer[DISPLAY_WIDTH*PARALLEL_LINES] = {0};

libesp::ScalingBuffer FrameBuf(&Display, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, uint8_t(16), DISPLAY_WIDTH,DISPLAY_HEIGHT, PARALLEL_LINES, (uint8_t*)&BackBuffer[0],(uint8_t*)&ParallelLinesBuffer[0]);

#define ESP_INTR_FLAG_DEFAULT 0

void initDisplay() {
	static const char *LOGTAG = "initDisplay";
	ESP_LOGI(LOGTAG,"Start initDisplay");
	spi_bus_config_t buscfg;
   buscfg.miso_io_num=PIN_NUM_DISPLAY_MISO;
   buscfg.mosi_io_num=PIN_NUM_DISPLAY_MOSI;
   buscfg.sclk_io_num=PIN_NUM_DISPLAY_CLK;
   buscfg.quadwp_io_num=-1;
   buscfg.quadhd_io_num=-1;
   buscfg.max_transfer_sz=10000;
   buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
   buscfg.intr_flags = 0;

	libesp::SPIBus::initializeBus(HSPI_HOST,buscfg,2);
	libesp::SPIBus* bus = libesp::SPIBus::get(HSPI_HOST);
	if(!bus) {
		ESP_LOGE(LOGTAG,"error initing BUS for touch");
	} else {
		ESP_LOGI(LOGTAG,"SPIBus initiatlized for display");
	}
   gpio_set_direction(PIN_NUM_DISPLAY_DATA_CMD, GPIO_MODE_OUTPUT);
	if(PIN_NUM_DISPLAY_RESET!=NOPIN) {
	  gpio_set_direction(PIN_NUM_DISPLAY_RESET, GPIO_MODE_OUTPUT);
	}
	if(PIN_NUM_DISPLAY_BACKLIGHT!=NOPIN) {
		gpio_set_direction(PIN_NUM_DISPLAY_BACKLIGHT, GPIO_MODE_OUTPUT);
	}
}
 
void initTouch() {
	static const char *LOGTAG = "initTouch";
	//touch bus config
	spi_bus_config_t buscfg;
   buscfg.miso_io_num=PIN_NUM_TOUCH_MISO;
   buscfg.mosi_io_num=PIN_NUM_TOUCH_MOSI;
   buscfg.sclk_io_num=PIN_NUM_TOUCH_CLK;
   buscfg.quadwp_io_num=-1;
   buscfg.quadhd_io_num=-1;
   buscfg.max_transfer_sz=64;
   buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
   buscfg.intr_flags = 0;

	libesp::SPIBus::initializeBus(VSPI_HOST,buscfg,1);
	libesp::SPIBus* bus = libesp::SPIBus::get(VSPI_HOST);
	if(!bus) {
		ESP_LOGE(LOGTAG,"error initing BUS for touch");
	}

}

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
	uint32_t gpio_num = 0;
	if(gpio_get_level(GPIO_NUM_0)==0) {
		gpio_num = GPIO_NUM_0;
	} else if (gpio_get_level(GPIO_NUM_39)==0) {
		gpio_num = GPIO_NUM_39;
	} else if (gpio_get_level(GPIO_NUM_36)==0) {
		gpio_num = GPIO_NUM_36;
	}
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
 
static void buttonHandler(void* arg) {
	uint32_t io_num;
	for(;;) {
		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			uint32_t level = (uint32_t) gpio_get_level((gpio_num_t)io_num);
			printf("GPIO[%d] intr, val: %d\n", io_num, level);
			//build test message
			libesp::System::get().logSystemInfo();	
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

//init Buttons
void initButtons() {
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	//start gpio task
	xTaskCreate(buttonHandler, "buttonHandler", 2048, NULL, 10, NULL);

	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	#define GPIO_INPUT_IO_39 (1ULL << GPIO_NUM_39)
	io_conf.pin_bit_mask = GPIO_INPUT_IO_39;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	gpio_isr_handler_add(GPIO_NUM_39, gpio_isr_handler, (void*) GPIO_INPUT_IO_39);
	
	#define GPIO_INPUT_IO_36 (1ULL << GPIO_NUM_36)
	io_conf.pin_bit_mask = GPIO_INPUT_IO_36;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	gpio_isr_handler_add(GPIO_NUM_36, gpio_isr_handler, (void*) GPIO_INPUT_IO_36);
	
	#define GPIO_INPUT_IO_0 (1ULL << GPIO_NUM_0)
	io_conf.pin_bit_mask = GPIO_INPUT_IO_0;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
}

void app_main() {
	static const char *LOGTAG="APP_MAIN";
	esp_err_t ret;
	
	// initialize NVS
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase()); // TODO: do we actually want this?
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	ESP32_I2CMaster::doIt();
	initTouch();

	libesp::SPIBus* bus = libesp::SPIBus::get(VSPI_HOST);
	TouchTask.init(bus,PIN_NUM_TOUCH_CS);
	TouchTask.start();
	///
	initDisplay();
	bus = libesp::SPIBus::get(HSPI_HOST);
	FrameBuf.createInitDevice(bus,PIN_NUM_DISPLAY_CS,PIN_NUM_DISPLAY_DATA_CMD);
	ESP_LOGI(LOGTAG,"start display init");
	libesp::ErrorType et=Display.init(libesp::DisplayILI9341::FORMAT_16_BIT, &Font_6x10, &FrameBuf);
	if(et.ok()) {
		ESP_LOGI(LOGTAG,"display init OK");
		Display.fillRec(0,0,FRAME_BUFFER_WIDTH,100,libesp::RGBColor::RED);
		//Display.fillRec(0,102,FRAME_BUFFER_WIDTH,152,libesp::RGBColor::BLUE);
		Display.swap();
		ESP_LOGI(LOGTAG,"display init swap done");
	} else {
		ESP_LOGE(LOGTAG,"failed display init");
	}
	///
	ESP_LOGI(LOGTAG,"setting up buttons");
	initButtons();
	/////
	ESP_LOGI(LOGTAG,"scan I2c0");
	ESP32_I2CMaster I2c(I2C_SCL,I2C_SDA,1000000, I2C_NUM_0, 0, 32);
	I2c.init(false);
	I2c.scan();
	//ESP_LOGI(LOGTAG,"scan I2c1");
	ESP32_I2CMaster I2c1(I2C_SCL2,I2C_SDA2,1000000, I2C_NUM_1, 0, 32);
	I2c1.init(true);
	//I2c1.scan();
	////
	// libbt.a is like 300kb
#define TEST_DISPLAY
#ifndef TEST_DISPLAY
	BTTask.init();
	BTTask.start();

	GameTask.init();
	GameTask.start();
	BTTask.setGameTaskQueue(GameTask.getQueueHandle());

	ExploitTask.init();
	ExploitTask.start();
	GameTask.installGame(EXPLOITABLE_ID, false, ExploitTask.getQueueHandle());
#endif
	// libwifi and friends is like 500kb
	//OTATask.init();
	//OTATask.start();
	//OTACmd* cmd = (OTACmd*)malloc(sizeof(OTACmd));
	//*cmd = ATTEMPT_OTA;
	//xQueueSend(OTATask.getQueueHandle(), &cmd, (TickType_t)100);

	libesp::System::get().logSystemInfo();	
}

