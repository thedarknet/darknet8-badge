#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include "./game_master.h"

const char* GAMEMASTER_LOGTAG = "GameMaster";

#define INSTALLED_GAMES (1)
bool unlocked_games[INSTALLED_GAMES];
// TODO: Game Objects

/*
	TODO: 

	Boolean array that dictates what is unlocked

	Command Queue available globally

	Menu item for pairing with each staff member

	CmdC0de: MUD Game
	Gourry:  Hackables

	Demo:    Echo w/ format string vulnerability (send in %s to echo a flag)
*/

bool GameTask::isGameUnlocked(uint8_t gameid)
{
	if (gameid >= INSTALLED_GAMES)
		return false;
	return unlocked_games[gameid];
}

bool GameTask::setGameUnlocked(uint8_t gameid)
{
	if (gameid >= INSTALLED_GAMES)
		return false;
	unlocked_games[gameid] = true;
	// TODO: Write to persistent storage
	return true;
}

void GameTask::commandHandler(GameMsg* msg)
{
	GameData* data = nullptr;
	GameMsg* omsg = nullptr;
	char* odata = nullptr;
	switch(msg->mtype)
	{
	case SGAME_RAW_INPUT:
		data = (GameData*)msg->data;
		/* TODO: Uncomment this when phone app starts sending stuff
		if ((data->context >= INSTALLED_GAMES) ||
			(!unlocked_games[data->context]))
		{
			esp_log_buffer_char(LOGTAG, msg->data, msg->length);
			ESP_LOGI(LOGTAG, "Attempted to access invalid or locked game\n");
			return; // Game is not unlocked
		}
		*/
		/* TODO
			Get game object
			Dispatch message to the game with response queue
		*/
		esp_log_buffer_char(LOGTAG, msg->data, msg->length);

		// TODO: Sample return message code, remove this
		if (msg->returnQueue)
		{
			omsg = (GameMsg*)malloc(sizeof(GameMsg));
			memset(omsg, '\0', sizeof(GameMsg));

			odata = (char*)malloc(4);
			memcpy(odata, "beep", 4);

			omsg->mtype = SGAME_RAW_OUTPUT;
			omsg->length = 4;
			omsg->data = odata;
			omsg->returnQueue = nullptr;
			xQueueSend(msg->returnQueue, (void*)&omsg, (TickType_t)100);
		}
		return;
	case SGAME_RAW_OUTPUT: // Should not occur
	case SGAME_UNKNOWN:
	default:
		return;
	}
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void GameTask::run(void* data)
{
	GameMsg* msg = nullptr;
	while(1)
	{
		if (xQueueReceive(GameQueueHandle, &msg, CmdQueueTimeout))
		{
			if (msg != nullptr)
			{
				this->commandHandler(msg);
				free(msg->data);
				free(msg);
			}
		}
	}
}

bool GameTask::init()
{
	this->GameQueueHandle = xQueueCreateStatic(GAME_QUEUE_SIZE, GAME_MSG_SIZE,
		gameQueueBuffer, &GameQueue);

	memset(&unlocked_games, '\0', sizeof(unlocked_games));
	// TODO: Check which games have been unlocked

	return true;
}

GameTask::GameTask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = GAMEMASTER_LOGTAG;
	ESP_LOGI(LOGTAG, "Created");
}

GameTask::~GameTask()
{
	// TODO:
	ESP_LOGI(LOGTAG, "DESTROY");
}
