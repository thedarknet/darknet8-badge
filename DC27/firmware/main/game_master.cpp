#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include <nvs.h>
#include <nvs_flash.h>

#include "./game_master.h"

const char* GAMEMASTER_LOGTAG = "GameMaster";

#define INSTALLED_GAMES (3)
bool unlocked_games[INSTALLED_GAMES];
// Game Interfaces are in the form of queues
QueueHandle_t game_queues[INSTALLED_GAMES];

// Helpers for sending responses
void SendResponse(GameMsg* msg, char* data, uint8_t size)
{
	// split into chunks?
	GameMsg* omsg = nullptr;
	if (msg->returnQueue)
	{
		omsg = (GameMsg*)malloc(sizeof(GameMsg));
		memset(omsg, '\0', sizeof(GameMsg));
		omsg->length = size;
		omsg->data = data;
		omsg->returnQueue = nullptr;
		omsg->returnContext = msg->returnContext;
		xQueueSend(msg->returnQueue, (void*)&omsg, (TickType_t)100);
	}
	else
		free(data);
	return;
}

void SendStringResponse(GameMsg* msg, const char* stringToCopy)
{
	int size = strlen(stringToCopy);
	char* tmp = (char*)malloc(size);
	memcpy(tmp, stringToCopy, size);
	SendResponse(msg, tmp, size);
	return;
}

void SendCopyResponse(GameMsg* msg, const char* copyme, uint8_t size)
{
	char* tmp = (char*)malloc(size);
	memcpy(tmp, copyme, size);
	SendResponse(msg, tmp, size);
	return;
}

// This function does a simple encoding function on your flag
// that way you don't store your flag in memory as plaintext
// this can be defeated if someone dumps memory and reverses out
// who is calling this function, but that's ok in my opinion
void SendWinResponse(GameMsg* msg, const char* a)
{
	char x[] = {"GOURRY"};
	int i = 0;
	char* b = (char*)malloc(strlen(a)+1);
	for (i = 0; i < strlen(a); i++)
	{
		b[i] = a[i] ^ x[i % 6];
		x[i % 6] = x[i % 6] + 1;
	}
	b[strlen(a)] = '\n';
	SendResponse(msg, b, strlen(a));
}

char base_tag[] = "game_unlocked";
char game_tag[20];
bool GameTask::isGameUnlocked(uint8_t gameid)
{
	bool unlocked = false;
	ESP_LOGI(LOGTAG, "Game unlocked check: %d", unlocked_games[gameid]);
	if (gameid >= INSTALLED_GAMES)
		return false;
	snprintf(game_tag, 20, "%s%d", base_tag, gameid);
	nvs_get_u8(this->my_nvs_handle, game_tag, (uint8_t*)&unlocked);
	ESP_LOGI(LOGTAG, "Game unlocked check: %d", unlocked);
	return unlocked;
}

bool GameTask::setGameUnlocked(uint8_t gameid)
{
	if (gameid >= INSTALLED_GAMES)
		return false;
	unlocked_games[gameid] = true;

	snprintf(game_tag, 20, "%s%d", base_tag, gameid);
	nvs_set_u8(this->my_nvs_handle, game_tag, (uint8_t)true);
	nvs_commit(this->my_nvs_handle);
	return true;
}

void GameTask::mainMenuSendResponse(GameMsg* msg, char* data, uint8_t size)
{
	// split into chunks?
	GameMsg* omsg = nullptr;
	if (msg->returnQueue)
	{
		omsg = (GameMsg*)malloc(sizeof(GameMsg));
		memset(omsg, '\0', sizeof(GameMsg));
		omsg->length = size;
		omsg->data = data;
		omsg->returnQueue = nullptr;
		omsg->returnContext = msg->returnContext;
		xQueueSend(msg->returnQueue, (void*)&omsg, (TickType_t)100);
	}
	else
		free(data);
	return;
}

void GameTask::sendBadContextError(GameMsg* msg)
{
	char* odata = (char*)malloc(27);
	memcpy(odata, "Bad Game Context Selection\n", 27);
	mainMenuSendResponse(msg, odata, 28);
	return;
}

void GameTask::sendGameLockedError(GameMsg* msg)
{
	char* odata = (char*)malloc(12);
	memcpy(odata, "Game Locked\n", 12);
	mainMenuSendResponse(msg, odata, 12);
	return;
}

void GameTask::mainMenu(GameMsg* msg)
{
	ESP_LOGI(LOGTAG, "Game Master");

	// TODO: Unlock codes for games

	return;
}

void GameTask::commandHandler(GameMsg* msg)
{
	ESP_LOGI(LOGTAG, "Message for Game: %x\n", msg->context);
	if ((msg->context >= INSTALLED_GAMES))
	{
		sendBadContextError(msg);
		goto cleanup;
	}
	else if (!isGameUnlocked(msg->context))
	{
		sendGameLockedError(msg);
		goto cleanup;
	}

	if (msg->context == 0)
		mainMenu(msg);
	else
	{
		QueueHandle_t game_queue = game_queues[msg->context];
		if (game_queue)
		{
			xQueueSend(game_queue, &msg, (TickType_t)100);
			return; // don't cleanup, the message needs to be retained
		}
		// if the game_queue is invalid, ditch this message
	}
cleanup:
	free(msg->data);
	free(msg);
	return;
}

bool GameTask::installGame(GameId id, bool unlocked, QueueHandle_t gameQueue)
{
	ESP_LOGI(LOGTAG, "Installing Game: %d", id);
	if (id > INSTALLED_GAMES)
	{
		ESP_LOGE(LOGTAG, "FAILED TO INSTALL, increment INSTALLED_GAMES in game_master.cpp");
		return false;
	}
	else if (!gameQueue)
	{
		ESP_LOGE(LOGTAG, "FAILED TO INSTALL, passed in null gameQueue");
		return false;
	}
	if (unlocked)
		setGameUnlocked(id);

	game_queues[id] = gameQueue;
	ESP_LOGI(LOGTAG, "Game Installed");
	return true;
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void GameTask::run(void* data)
{
	GameMsg* msg = nullptr;
	while(1)
	{
		if (xQueueReceive(this->GameQueueHandle, &msg, CmdQueueTimeout))
		{
			if (msg != nullptr)
				this->commandHandler(msg);
		}
	}
}

bool GameTask::init()
{
	int i = 0;
	bool unlocked;
	this->GameQueueHandle = xQueueCreateStatic(GAME_QUEUE_SIZE, GAME_MSG_SIZE,
		gameQueueBuffer, &GameQueue);

	memset(unlocked_games, '\0', sizeof(unlocked_games));
	unlocked_games[0] = true; // GameMaster Menu is always unlocked

	// Game Master
	game_queues[GAMEMASTER_ID] = nullptr; // unused

	// retrieve locked status
	if (nvs_open("storage", NVS_READWRITE, &this->my_nvs_handle) != ESP_OK)
	{
		ESP_LOGE(LOGTAG, "FAILED TO OPEN NVS");
		return false;
	}

	for (i = 0; i < INSTALLED_GAMES; i++)
	{
		if (i == 0) continue; // ignore game-master, always unlocked
		unlocked = false;
		snprintf(game_tag, 20, "%s%d", base_tag, i);
		if (nvs_get_u8(this->my_nvs_handle, game_tag, (uint8_t*)&unlocked) == ESP_OK)
		{
			ESP_LOGI(LOGTAG, "game_tag: %s. unlocked %d", game_tag, unlocked);
			unlocked_games[i] = unlocked;
		}
		else
		{
			unlocked_games[i] = false;
			nvs_set_u8(this->my_nvs_handle, game_tag, (uint8_t)unlocked);
			nvs_commit(this->my_nvs_handle);
		}
	}

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
