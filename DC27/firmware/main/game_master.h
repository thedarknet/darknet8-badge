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
	BRAINFUZZ_ID   = 0x02,
	TTT3D_ID       = 0x03,

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


void SendResponse(GameMsg* msg, char* data, uint8_t size);
void SendStringResponse(GameMsg* msg, const char* stringToCopy);
void SendCopyResponse(GameMsg* msg, const char* copyme, uint8_t size);
void SendWinResponse(GameMsg* msg, const char* a);

class GameTask : public Task {
private:

	// TTT3D Resources
	bool T3DverifyInput(GameMsg* msg);
	void T3DprintBoard(GameMsg* msg);
	bool T3Dtimeout(GameMsg* msg);
	int  T3DscoreBoard(GameMsg* msg);
	void T3DmakePlay(GameMsg* msg);
	void T3DnewGame(GameMsg* msg);
	void T3Dreset(GameMsg* msg);
	void T3DcommandHandler(GameMsg* msg);
	bool T3Dinit(void);

	long int t3_start_time = 0;
	int      t3_net_wins = 0;
	bool     t3_timer_started = false;
	int      t3_x, t3_y, t3_z;
	int      t3_moves = 0;
	char     t3_board[3][3][3];

	// Brainfuzz Resources
	void BFreset();
	bool BFverifyInput(GameMsg* msg);
	void BFrun(GameMsg* msg);
	void BFcommandHandler(GameMsg* msg);

	char bf_buf[512];
	char bf_output[512];
	uint16_t bf_output_length;

	// Exploitable Game Resources
	void EXPsetCurrentLevel(uint32_t lvlid);
	void EXPcommandHandler(GameMsg* msg);
	void EXPinit();

	void*    exp_levels[8];
	uint32_t exp_cur_level = 0;
	
	// GameMaster resources

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

	bool installGame(GameId id, bool unlocked);

	void mainMenuSendResponse(GameMsg* msg, char* data, uint8_t size);
	void sendBadContextError(GameMsg* msg);
	void sendGameLockedError(GameMsg* msg);
	void mainMenu(GameMsg* msg);
};

#endif //__SERIAL_GAME__
