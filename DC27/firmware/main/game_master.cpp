#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include "./game_master.h"

// XXX: Include your game here, initialize it in GameTask.init
#include "exploitable.h"

const char* GAMEMASTER_LOGTAG = "GameMaster";

typedef struct
{
	GameDataType dtype;
	char data;
} GameMasterData;


#define INSTALLED_GAMES (2)
bool unlocked_games[INSTALLED_GAMES];
// Game Interfaces are in the form of queues
QueueHandle_t game_queues[INSTALLED_GAMES];

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
static char flag[] = "Very Good! Now Echo This: ZOOP";
static char exploit_unlocked[] = "Exploitable Game Unlocked\n";
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
		if (!orig_length) // must have at least 1 byte to echo
			return;
		if (!strncmp("ZOOP", &gdata->data, 4))
		{
			ESP_LOGI(LOGTAG, "GAME_ACTION: ZOOP - unlocking exploitable quest");
			tmp = (char*)malloc(strlen(exploit_unlocked));
			memcpy(tmp, exploit_unlocked, strlen(exploit_unlocked));
			// TODO: Actualy unlock it
			setGameUnlocked(EXPLOITABLE_ID);
			mainMenuSendResponse(msg, tmp, strlen(exploit_unlocked));
		} 
		else
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

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void GameTask::run(void* data)
{
	GameMsg* msg = nullptr;
	while(1)
	{
		if (xQueueReceive(GameQueueHandle, &msg, CmdQueueTimeout))
		{
			if (msg != nullptr)
				this->commandHandler(msg);
		}
	}
}

ExploitableGameTask ExploitTask("ExploitTask");

bool GameTask::init()
{
	this->GameQueueHandle = xQueueCreateStatic(GAME_QUEUE_SIZE, GAME_MSG_SIZE,
		gameQueueBuffer, &GameQueue);

	memset(&unlocked_games, '\0', sizeof(unlocked_games));
	unlocked_games[0] = true; // GameMaster Menu is always unlocked

	// Game Master
	game_queues[GAMEMASTER_ID] = nullptr; // unused

	// Initialize and start the installed games
	// Exploitable
	ExploitTask.init();
	ExploitTask.start();
	game_queues[EXPLOITABLE_ID] = ExploitTask.getQueueHandle();

	// TODO: Check which games have been unlocked, stored in hardware

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
