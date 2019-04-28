#ifndef __SERIAL_GAME__
#define __SERIAL_GAME__

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "libesp/Task.h"

QueueHandle_t GetSerialGameMsgQueue(void);

typedef enum
{
	// Inbound
	SGAME_GET_MENU,
	SGAME_RAW_INPUT,

	// Outbound
	SGAME_MENU_LINE,
	SGAME_RAW_OUTPUT,

	SGAME_UNKNOWN
} GameMsgType;

typedef struct
{
	GameMsgType mtype;
	uint16_t length;
	char* data;
	QueueHandle_t returnQueue;
} SerialGameMsg;

class SerialGameTask : public Task {
private:
	static const int GAME_QUEUE_SIZE = 3;
	static const int GAME_MSG_SIZE = sizeof(SerialGameMsg);
	StaticQueue_t GameQueue;
	QueueHandle_t GameQueueHandle = nullptr;
	uint8_t gameQueueBuffer[GAME_QUEUE_SIZE*GAME_MSG_SIZE];

	void commandHandler(SerialGameMsg* msg);

public:
	const char *LOGTAG;
	SerialGameTask(const std::string &tName, uint16_t stackSize=10000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~SerialGameTask();
	QueueHandle_t getQueueHandle() {return GameQueueHandle;}
};

#endif //__SERIAL_GAME__
