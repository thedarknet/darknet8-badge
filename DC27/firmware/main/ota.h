#ifndef DC27_OTA
#define DC27_OTA

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <libesp/task.h>
#include <nvs_flash.h>
#include <nvs.h>

typedef enum
{
	ATTEMPT_OTA   = 0x00,
	KILL_OTA      = 0x01,
	FACTORY_RESET = 0x02,
	SELECT_BOOT_PARTITION = 0x03,

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

	uint32_t my_nvs_handle;

	void do_factory_reset(void);

public:
	static const char *LOGTAG;
	OTATask(const std::string &tName, uint16_t stackSize=10000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~OTATask();

	QueueHandle_t getQueueHandle() {return OTAQueueHandle;}
};

#endif // DC27_OTA
