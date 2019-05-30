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

#define I2C_SCL GPIO_NUM_22
#define I2C_SDA GPIO_NUM_21

extern "C" {
	void app_main();
}

BluetoothTask BTTask("BluetoothTask");
GameTask GameTask("GameTask");
ExploitableGameTask ExploitTask("ExploitTask");
//OTATask OTATask("OTATask");
libesp::XPT2046 TouchTask(4,25,PIN_NUM_TOUCH_IRQ);

#define ESP_INTR_FLAG_DEFAULT 0
 
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

void app_main() {
	esp_err_t ret;
	
	// initialize NVS
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase()); // TODO: do we actually want this?
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	//ESP32_I2CMaster::doIt();
	initTouch();
	libesp::SPIBus* bus = libesp::SPIBus::get(VSPI_HOST);
	TouchTask.init(bus,PIN_NUM_TOUCH_CS);
	TouchTask.start();
	/////
	ESP32_I2CMaster I2c(I2C_SCL,I2C_SDA,1000000, I2C_NUM_0, 0, 32);
	I2c.init(false);
	I2c.scan();
	////
	// libbt.a is like 300kb
	BTTask.init();
	BTTask.start();

	GameTask.init();
	GameTask.start();
	BTTask.setGameTaskQueue(GameTask.getQueueHandle());

	ExploitTask.init();
	ExploitTask.start();
	GameTask.installGame(EXPLOITABLE_ID, false, ExploitTask.getQueueHandle());

	// libwifi and friends is like 500kb
	//OTATask.init();
	//OTATask.start();
	//OTACmd* cmd = (OTACmd*)malloc(sizeof(OTACmd));
	//*cmd = ATTEMPT_OTA;
	//xQueueSend(OTATask.getQueueHandle(), &cmd, (TickType_t)100);

	libesp::System::get().logSystemInfo();	
}

