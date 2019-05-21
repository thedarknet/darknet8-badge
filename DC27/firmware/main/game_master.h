#ifndef __SERIAL_GAME__
#define __SERIAL_GAME__

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "libesp/task.h"

QueueHandle_t GetGameMsgQueue(void);

typedef enum
{
	// Inbound
	SGAME_RAW_INPUT,

	// Outbound
	SGAME_RAW_OUTPUT,

	SGAME_UNKNOWN
} GameMsgType;

typedef enum
{
	GAME_INIT = 0x00,
	GAME_ACTION = 0x01,

	GAME_UNKNOWN = 0xFF
} GameDataType;

typedef struct
{
	GameDataType dtype;
	uint16_t length;
	char* data;
} GameData;

typedef struct
{
	uint8_t context;
	GameMsgType mtype;
	uint16_t length;
	char* data;
	QueueHandle_t returnQueue;
} GameMsg;

class GameTask : public Task {
private:
	static const int GAME_QUEUE_SIZE = 3;
	static const int GAME_MSG_SIZE = sizeof(GameMsg);
	StaticQueue_t GameQueue;
	QueueHandle_t GameQueueHandle = nullptr;
	uint8_t gameQueueBuffer[GAME_QUEUE_SIZE*GAME_MSG_SIZE];

	void commandHandler(GameMsg* msg);

public:
	const char *LOGTAG;
	GameTask(const std::string &tName, uint16_t stackSize=10000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~GameTask();
	QueueHandle_t getQueueHandle() {return GameQueueHandle;}

	bool isGameUnlocked(uint8_t game_id);
	bool setGameUnlocked(uint8_t game_id);

	void mainMenuSendResponse(GameMsg* msg, char* data, uint8_t size);
	void sendBadContextError(GameMsg* msg);
	void sendGameLockedError(GameMsg* msg);
	void mainMenu(GameMsg* msg);
};

#endif //__SERIAL_GAME__
