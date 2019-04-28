#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include "./serial_game.h"

const char* SERIAL_LOGTAG = "SerialGame";

/*
	TODO: 

	Boolean array that dictates what is unlocked

	Command Queue available globally

	Menu item for pairing with each staff member

	CmdC0de: MUD Game
	Gourry:  Hackables

	Demo:    Echo w/ format string vulnerability (send in %s to echo a flag)
*/

void SerialGameTask::commandHandler(SerialGameMsg* msg)
{
	switch(msg->mtype)
	{
	case SGAME_GET_MENU:
		// TODO:
		return;
	case SGAME_RAW_INPUT:
		// TODO
		esp_log_buffer_char(LOGTAG, msg->data, msg->length);
		return;
	case SGAME_MENU_LINE: // Should not occur
	case SGAME_RAW_OUTPUT: // Should not occur
	case SGAME_UNKNOWN:
	default:
		return;
	}
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void SerialGameTask::run(void* data)
{
	SerialGameMsg* msg = nullptr;
	while(1)
	{
		if (xQueueReceive(GameQueueHandle, &msg, CmdQueueTimeout))
		{
			if (msg != nullptr)
			{
				this->commandHandler(msg);
				free(msg);
			}
		}
	}
}

bool SerialGameTask::init()
{
	this->GameQueueHandle = xQueueCreateStatic(GAME_QUEUE_SIZE, GAME_MSG_SIZE,
		gameQueueBuffer, &GameQueue);
	return true;
}

SerialGameTask::SerialGameTask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = SERIAL_LOGTAG;
	ESP_LOGI(LOGTAG, "Created");
}

SerialGameTask::~SerialGameTask()
{
	// TODO:
	ESP_LOGI(LOGTAG, "DESTROY");
}
