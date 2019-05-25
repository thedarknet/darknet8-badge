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

typedef struct
{
	GameDataType dtype;
	char data;
} GameMasterData;

#define INSTALLED_GAMES (2)
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
	// split into chunks?
	GameMsg* omsg = nullptr;
	if (msg->returnQueue)
	{
		omsg = (GameMsg*)malloc(sizeof(GameMsg));
		memset(omsg, '\0', sizeof(GameMsg));
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
	char* odata = (char*)malloc(28);
	memcpy(odata, "Bad Game Context Selection\n\0", 28);
	mainMenuSendResponse(msg, odata, 28);
	return;
}

void GameTask::sendGameLockedError(GameMsg* msg)
{
	char* odata = (char*)malloc(13);
	memcpy(odata, "Game Locked\n\0", 13);
	mainMenuSendResponse(msg, odata, 13);
	return;
}

static char mainMenu_str[] = "I am the game master, speak and I shall echo...\n";
static char flag[] = "Very Good! {Flag: ZOOP}";
void GameTask::mainMenu(GameMsg* msg)
{
	GameMasterData* gdata = (GameMasterData*)msg->data;
	char* tmp = nullptr;
	char* ftmp = nullptr;
	int orig_length = 0;
	int tmp_length = 0;
	ESP_LOGI(LOGTAG, "Game Master");
	if (gdata->dtype == GAME_INIT)
	{
		ESP_LOGI(LOGTAG, "GAME_INIT");
		// respond with main menu
		tmp = (char*)malloc(strlen(mainMenu_str));
		memcpy(tmp, mainMenu_str, strlen(mainMenu_str));
		mainMenuSendResponse(msg, tmp, strlen(mainMenu_str));
	}
	else if (gdata->dtype == GAME_ACTION)
	{
		orig_length = msg->length - 4; // subtrace dtype
		if (orig_length) // must have at least 1 byte to echo
		{
			ESP_LOGI(LOGTAG, "GAME_ACTION: echo %s, length %d", &gdata->data, orig_length);
			// Prevent corruption by extending the buffer to the length of the flag
			// Technically corruption can still happen with %n... but hey that's up
			// to them to figure it out.  They could unlock all the games this way
			// if they wanted.  But I'm guessing it would take them way too long
			// to actually figure it out, because string format vulns are a huge pain
			// in the ass.
			tmp_length = (strlen(flag) > orig_length) ? strlen(flag) : orig_length;
			tmp = (char*)malloc(tmp_length); // add \n and null
			snprintf(tmp, tmp_length, &gdata->data, flag);
			// We've copied some amount of data, but only echo back the size of the
			// original message.  Not as easy as passing in just %s!
			ftmp = (char*)malloc(orig_length + 2);
			memcpy(ftmp, tmp, orig_length);
			ftmp[orig_length] = '\n';
			ftmp[orig_length + 1] = 0x0;
			free(tmp);

			mainMenuSendResponse(msg, ftmp, orig_length + 2);
		}
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
	ESP_LOGI(LOGTAG, "Message for Game: %x\n", msg->context);
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
