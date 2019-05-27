#ifndef DC27_OTA
#define DC27_OTA

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <libesp/task.h>

typedef enum
{
	ATTEMPT_OTA = 0x00,
	KILL_OTA    = 0x01,

	DEFAULT_OTA = 0xFF
} OTACmd;

class OTATask : public Task {
private:
	QueueHandle_t gameTaskQueue = nullptr;

	static const int OTA_QUEUE_SIZE = 3;
	static const int OTA_MSG_SIZE = sizeof(OTACmd);
	StaticQueue_t OTAQueue;
	QueueHandle_t OTAQueueHandle = nullptr;
	uint8_t otaQueueBuffer[OTA_QUEUE_SIZE * OTA_MSG_SIZE];

public:
	static const char *LOGTAG;
	OTATask(const std::string &tName, uint16_t stackSize=10000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~OTATask();

	QueueHandle_t getQueueHandle() {return OTAQueueHandle;}
};

#endif // DC27_OTA
