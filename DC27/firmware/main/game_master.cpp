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

	General: GameMaster Interface
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

void GameTask::mainMenuSendResponse(GameMsg* msg, char* data, uint8_t size)
{
	
	GameMsg* omsg = nullptr;
	if (msg->returnQueue)
	{
		omsg = (GameMsg*)malloc(sizeof(GameMsg));
		memset(omsg, '\0', sizeof(GameMsg));
		omsg->mtype = SGAME_RAW_OUTPUT;
		omsg->length = size;
		omsg->data = data;
		omsg->returnQueue = nullptr;
		xQueueSend(msg->returnQueue, (void*)&omsg, (TickType_t)100);
	}
	else
		free(data);
	return;
}

void GameTask::sendBadContextError(GameMsg* msg)
{
	char* odata = (char*)malloc(26);
	memcpy(odata, "Bad Game Context Selection", 26);
	mainMenuSendResponse(msg, odata, 26);
	return;
}

void GameTask::sendGameLockedError(GameMsg* msg)
{
	char* odata = (char*)malloc(11);
	memcpy(odata, "Game Locked", 11);
	mainMenuSendResponse(msg, odata, 11);
	return;
}

static char mainMenu_str[] = "Main Menu:";

void GameTask::mainMenu(GameMsg* msg)
{
	GameData* data = (GameData*)msg->data;
	char* tmp = nullptr;
	ESP_LOGI(LOGTAG, "Main Menu");
	if (data->dtype == GAME_INIT)
	{
		ESP_LOGI(LOGTAG, "GAME_INIT");
		// respond with main menu
		tmp = (char*)malloc(strlen(mainMenu_str));
		memcpy(tmp, mainMenu_str, strlen(mainMenu_str));
		mainMenuSendResponse(msg, tmp, strlen(mainMenu_str));
	}
	else if (data->dtype == GAME_ACTION)
	{
		ESP_LOGI(LOGTAG, "GAME_ACTION");
		// check msg length (1 byte max)
		// make a game selection
		// send game context control setting
	}
	else
	{
		ESP_LOGI(LOGTAG, "Unknown GameData type");
		esp_log_buffer_char(LOGTAG, msg->data, msg->length);
	}
	return;
}

void GameTask::commandHandler(GameMsg* msg)
{
	switch(msg->mtype)
	{
	case SGAME_RAW_INPUT:
		if ((msg->context >= INSTALLED_GAMES))
			return sendBadContextError(msg);
		else if (!isGameUnlocked(msg->context))
			return sendGameLockedError(msg);

		if (msg->context == 0)
			return mainMenu(msg);
		else
		{
			/* TODO
				Get game object
				Dispatch message to the game with response queue
			*/
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
	unlocked_games[0] = true; // GameMaster Menu is always unlocked
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
