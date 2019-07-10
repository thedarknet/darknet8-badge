/*
 * DC26.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: dcomes
 */

#include <stdint.h>
#include "buttons.h"
#include "driver/gpio.h"
#include "libesp/freertos.h"

using libesp::FreeRTOS;
/*
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
*/

ButtonInfo::ButtonInfo() :
		ButtonState(0),LastButtonState(0), LastTickButtonPushed(0) {
	LastTickButtonPushed = FreeRTOS::getTimeSinceStart();
}
//init Buttons
bool ButtonInfo::init() {
	//gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	//start gpio task
	//xTaskCreate(buttonHandler, "buttonHandler", 2048, NULL, 10, NULL);

	gpio_config_t io_conf;
	//io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	#define GPIO_INPUT_IO_39 (1ULL << GPIO_NUM_39)
	io_conf.pin_bit_mask = GPIO_INPUT_IO_39;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	//gpio_isr_handler_add(GPIO_NUM_39, gpio_isr_handler, (void*) GPIO_INPUT_IO_39);
	
	#define GPIO_INPUT_IO_36 (1ULL << GPIO_NUM_36)
	io_conf.pin_bit_mask = GPIO_INPUT_IO_36;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	//gpio_isr_handler_add(GPIO_NUM_36, gpio_isr_handler, (void*) GPIO_INPUT_IO_36);
	
	#define GPIO_INPUT_IO_0 (1ULL << GPIO_NUM_0)
	io_conf.pin_bit_mask = GPIO_INPUT_IO_0;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	//gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
	return true;
}

void ButtonInfo::reset() {
	ButtonState = LastButtonState = 0;
}

bool ButtonInfo::areTheseButtonsDown(const int32_t &b) {
	return (ButtonState & b) == b;
}

bool ButtonInfo::isAnyOfTheseButtonDown(const int32_t &b) {
	return (ButtonState&b)!=0;
}

bool ButtonInfo::isAnyButtonDown() {
	return ButtonState!=0;
}

bool ButtonInfo::wereTheseButtonsReleased(const int32_t &b) {
	//last state must match these buttons and current state must have none of these buttons
	return (LastButtonState & b) == b && (ButtonState&b)==0;
}

bool ButtonInfo::wereAnyOfTheseButtonsReleased(const int32_t &b) {
	//last state must have at least 1 of the buttons and at least 1 of the buttons must not be down now
	return (LastButtonState & b) != 0 && !isAnyOfTheseButtonDown(b);
}

bool ButtonInfo::wasAnyButtonReleased() {
	return ButtonState!=LastButtonState && LastButtonState!=0;
}


void ButtonInfo::process() {
	LastButtonState = ButtonState;
	ButtonState = 0;
	if (gpio_get_level(GPIO_NUM_39) == 0) {
		ButtonState|=BUTTON_RIGHT_DOWN;
	}
	if (gpio_get_level(GPIO_NUM_36) == 0) {
		ButtonState|=BUTTON_LEFT_UP;
	}
	if (gpio_get_level(GPIO_NUM_0) == 0) {
		ButtonState|=BUTTON_FIRE1;
	}
	if(ButtonState!=0) {
		LastTickButtonPushed = FreeRTOS::getTimeSinceStart();
	}
}

uint32_t ButtonInfo::lastTickButtonPushed() {
	return LastTickButtonPushed;
}

