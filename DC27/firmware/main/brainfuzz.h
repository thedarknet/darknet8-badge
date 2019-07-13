#ifndef __BRAINFUZZ_GAME__
#define __BRAINFUZZ_GAME__

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "libesp/task.h"

#include "game_master.h"

QueueHandle_t GetBrainfuzzGameQueue(void);

class BrainfuzzGameTask : public Task {
private:
	static const int GAME_QUEUE_SIZE = 3;
	static const int GAME_MSG_SIZE = sizeof(GameMsg);
	StaticQueue_t GameQueue;
	QueueHandle_t GameQueueHandle = nullptr;
	uint8_t gameQueueBuffer[GAME_QUEUE_SIZE*GAME_MSG_SIZE];
	void commandHandler(GameMsg* msg);
	void reset();
	void runBrainfuzz(GameMsg* msg);

	char buf[512];
	char output[512];
	uint16_t output_length;
public:
	const char *LOGTAG;
	BrainfuzzGameTask(const std::string &tName, uint16_t stackSize=3000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~BrainfuzzGameTask();
	QueueHandle_t getQueueHandle() {return GameQueueHandle;}
};

#endif //__BRAINFUZZ_GAME__
