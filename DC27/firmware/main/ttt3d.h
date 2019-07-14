#ifndef __TTT3D_GAME__
#define __TTT3D_GAME__

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "libesp/task.h"

#include "game_master.h"

QueueHandle_t GetTTT3DGameQueue(void);

class TTT3DGameTask : public Task {
private:
	static const int GAME_QUEUE_SIZE = 3;
	static const int GAME_MSG_SIZE = sizeof(GameMsg);
	StaticQueue_t GameQueue;
	QueueHandle_t GameQueueHandle = nullptr;
	uint8_t gameQueueBuffer[GAME_QUEUE_SIZE*GAME_MSG_SIZE];
	bool verifyInput(GameMsg* msg);
	void printBoard(GameMsg* msg);
	bool timeout(GameMsg* msg);
	int scoreBoard(GameMsg* msg);
	void makePlay(GameMsg* msg);
	void newGame(GameMsg* msg);
	void reset(GameMsg* msg);
	void commandHandler(GameMsg* msg);
	void runTTT3D(GameMsg* msg);

	int net_wins = 0;
	long int start_time = 0;
	bool timer_started = false;
	int x, y, z;
	int moves = 0;
	char board[3][3][3];

public:
	const char *LOGTAG;
	TTT3DGameTask(const std::string &tName, uint16_t stackSize=3000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~TTT3DGameTask();
	QueueHandle_t getQueueHandle() {return GameQueueHandle;}
};

#endif //__TTT3D_GAME__
