#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"

#include "system.h"
#include "i2c.hpp"
#include <esp_log.h>

#include "nvs_flash.h"

#include "app.h"

//#define DEVKIT_I2C

#ifdef DEVKIT_I2C
#define I2C_SCL NOPIN
#define I2C_SDA NOPIN
#else
#define I2C_SCL GPIO_NUM_22
#define I2C_SDA GPIO_NUM_21
#endif
#define I2C_SCL2 GPIO_NUM_2
#define I2C_SDA2 GPIO_NUM_16


extern "C" {
	void app_main();
}

#define ESP_INTR_FLAG_DEFAULT 0

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

	
	libesp::ErrorType et;
	et = DN8App::get().init();

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"init error: %s", et.toString());
	}


	//ESP32_I2CMaster::doIt();

	/////
	//ESP_LOGI(LOGTAG,"scan I2c0");
	//ESP32_I2CMaster I2c(I2C_SCL,I2C_SDA,1000000, I2C_NUM_0, 0, 32);
	//I2c.init(false);
	//I2c.scan();
	//ESP_LOGI(LOGTAG,"scan I2c1");
	ESP32_I2CMaster I2c1(I2C_SCL2,I2C_SDA2,1000000, I2C_NUM_1, 0, 32);
	I2c1.init(true);
	//I2c1.scan();
	

	libesp::System::get().logSystemInfo();

   do {
		et = DN8App::get().run();
		//vTaskDelay(1 / portTICK_RATE_MS);
	} while (et.ok());
	vTaskDelay(3000 / portTICK_RATE_MS);
}

