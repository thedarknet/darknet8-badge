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

#include "nvs_flash.h"

#include "app.h"



#define I2C_SCL GPIO_NUM_22
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL2 GPIO_NUM_2
#define I2C_SDA2 GPIO_NUM_16


extern "C" {
	void app_main();
}

#define ESP_INTR_FLAG_DEFAULT 0

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

	libesp::ErrorType et;
	et = DN8App::get().init();

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"init error: %s", et.toString());
	}


	ESP32_I2CMaster::doIt();

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
	

	libesp::System::get().logSystemInfo();

	vTaskDelay(3000 / portTICK_RATE_MS);
	DN8App::get().run();
}

