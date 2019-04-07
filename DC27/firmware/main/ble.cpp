
#include "ble.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <map>
#include <nvs_flash.h>

BluetoothTask *pBTTask;
const char *BluetoothTask::LOGTAG = "BluetoothTask";

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void BluetoothTask::run(void* data)
{
	ESP_LOGI(LOGTAG, "RUNNING");
	while (1)
	{
		//ESP_LOGI(LOGTAG, "BOOP");
		vTaskDelay(CmdQueueTimeout);
	}
	ESP_LOGI(LOGTAG, "COMPLETE");
}


bool BluetoothTask::init()
{
	ESP_LOGI(LOGTAG, "INIT START");
	pBTTask = this;

	ESP_LOGI(LOGTAG, "INIT COMPLETE");
	return true;
}

BluetoothTask::BluetoothTask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = LOGTAG;
	ESP_LOGI(LOGTAG, "CREATE");
}

BluetoothTask::~BluetoothTask()
{
	// TODO
	ESP_LOGI(LOGTAG, "DESTROY");
}
