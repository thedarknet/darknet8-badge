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
	GAMEMASTER_ID  = 0x00,
	EXPLOITABLE_ID = 0x01,

	INVALID_ID     = 0xFF,
} GameId;

typedef struct
{
	uint8_t context;
	uint8_t length;
	char* data;
	QueueHandle_t returnQueue;
	uint32_t returnContext;
} GameMsg;

class GameTask : public Task {
private:
	static const int GAME_QUEUE_SIZE = 3;
	static const int GAME_MSG_SIZE = sizeof(GameMsg);
	StaticQueue_t GameQueue;
	QueueHandle_t GameQueueHandle = nullptr;
	uint8_t gameQueueBuffer[GAME_QUEUE_SIZE*GAME_MSG_SIZE];

	uint32_t my_nvs_handle = 0;

	void commandHandler(GameMsg* msg);

public:
	const char *LOGTAG;
	GameTask(const std::string &tName, uint16_t stackSize=3000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~GameTask();
	QueueHandle_t getQueueHandle() {return GameQueueHandle;}

	bool isGameUnlocked(uint8_t game_id);
	bool setGameUnlocked(uint8_t game_id);

	bool installGame(GameId id, bool unlocked, QueueHandle_t gameQueue);

	void mainMenuSendResponse(GameMsg* msg, char* data, uint8_t size);
	void sendBadContextError(GameMsg* msg);
	void sendGameLockedError(GameMsg* msg);
	void mainMenu(GameMsg* msg);
};

#endif //__SERIAL_GAME__
