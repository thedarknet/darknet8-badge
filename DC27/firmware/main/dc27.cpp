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
#include "cryptoauthlib.h"


#define I2C_SCL GPIO_NUM_22
#define I2C_SDA GPIO_NUM_21
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

	ATCAIfaceCfg *gCfg = &cfg_ateccx08a_i2c_default;
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t random_number[32];
	uint8_t serial_number[9];

	gCfg->iface_type = ATCA_I2C_IFACE,
	gCfg->devtype = ATECC608A,
	gCfg->atcai2c.slave_address = 0xC0;  // Detected correctly!!
	gCfg->atcai2c.bus = 0;
	gCfg->atcai2c.baud = 400000;
	gCfg->wake_delay = 800;
	gCfg->rx_retries = 20;

	status = atcab_init(gCfg);

	if (status == ATCA_SUCCESS) {
		uint8_t ret[4];
		status = atcab_selftest(SELFTEST_MODE_ALL,0,&ret[0]);
		ESP_LOGI(LOGTAG, "selft test status %d, result: %d",status, int32_t(ret[0]));
		  
		status = atcab_info(&ret[0]);
		ESP_LOGI(LOGTAG, "info status %d",status );
		ESP_LOG_BUFFER_HEX(LOGTAG, &ret[0],sizeof(ret));

		uint32_t counter = 0;
		status = atcab_counter_read(1,&counter);
		ESP_LOGI(LOGTAG, "counter status %d, counter %d",status, counter );
		status = atcab_counter_increment(1,&counter);
		ESP_LOGI(LOGTAG, "counter status %d, counter %d",status, counter );
		status = atcab_counter_increment(1,&counter);
		ESP_LOGI(LOGTAG, "counter status %d, counter %d",status, counter );

		status = atcab_random(random_number);
		ESP_LOGI(LOGTAG, "\tRandom: %i", status);
		ESP_LOG_BUFFER_HEX(LOGTAG, &random_number[0],sizeof(random_number));

		status = atcab_read_serial_number(serial_number);
		ESP_LOGI(LOGTAG, "\tSerial: %i", status);
		ESP_LOG_BUFFER_HEX(LOGTAG, &serial_number[0],sizeof(serial_number));

		char s[10] = {'\0'};
		status = atcab_version(s);
		ESP_LOGI(LOGTAG, "version: status %d, version %s",status, s );
		//status = atcab_release();
		//ESP_LOGI(LOGTAG, "\tRelease: %i", status);
    } else {
        ESP_LOGE(LOGTAG, "\t- ERROR: %i", status);
    }
	
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

