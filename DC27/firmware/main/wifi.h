#ifndef DC27_WIFI
#define DC27_WIFI

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "task.h"
#include <nvs_flash.h>
#include <nvs.h>

#include "lib/WiFi.h"


typedef enum
{
	WIFI_OK            = 0x00, // Request 
	WIFI_SCAN_RESP     = 0x01, // Found someone, returning data
	WIFI_SCAN_COMPLETE = 0x02, // No longer scanning

	// WIFI_ERR_x = 0x8_,
	WIFI_ERR_CONNECTAP = 0x80,
	WIFI_ERR_UNK = 0xFF
} WIFIResponseType;

typedef struct ScanResult
{
	char* bssid;
	char* ssid;
	wifi_auth_mode_t authmode;
} ScanResult;

typedef struct WIFIResponseMsg
{
	WIFIResponseType type;
	ScanResult sres;
} WIFIResponseMsg;

typedef enum
{
	WIFI_ATTEMPT_OTA  = 0x00,
	WIFI_AP_START     = 0x01,
	WIFI_AP_STOP      = 0x02,
	WIFI_SCAN         = 0x03,
	WIFI_NPC_INTERACT = 0x04,

} WIFICmd;

typedef enum
{
	NPC_LIST   = 0x00,
	NPC_ACTION = 0x01
} NpcMsgType;

typedef struct WifiNpcMsg
{
	NpcMsgType type;
	char* npcname;
	char* ssid;
	uint8_t* data;
} WifiNpcMsg;

typedef struct WifiScanMsg
{
	bool npcfilter;
} WifiScanMsg;

typedef struct StartAPMsg
{
	wifi_auth_mode_t mode;
	char* ssid;
	char* passwd;
} StartAPMsg;

typedef struct WIFIMsg
{
	WIFICmd cmd;
	union
	{
		WifiNpcMsg wnm;
		WifiScanMsg wsm;
		StartAPMsg sap;
	} data;
	QueueHandle_t returnQueue;
} WIFIMsg;

class WIFITask : public Task {
private:
	QueueHandle_t gameTaskQueue = nullptr;

	static const int WIFI_QUEUE_SIZE = 3;
	static const int WIFI_MSG_SIZE = sizeof(WIFIMsg);
	StaticQueue_t WIFIQueue;
	QueueHandle_t WIFIQueueHandle = nullptr;
	uint8_t otaQueueBuffer[WIFI_QUEUE_SIZE * WIFI_MSG_SIZE];
	uint32_t my_nvs_handle;

	void wifiScan(WIFIMsg* msg);
	void npcInteract(WIFIMsg* msg);
	void startAp(WIFIMsg* msg);
	void stopAp(WIFIMsg* msg);

protected:
	WiFi wifi;

public:
	static const char *LOGTAG;
	WIFITask(const std::string &tName, uint16_t stackSize=10000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~WIFITask();

	QueueHandle_t getQueueHandle() {return WIFIQueueHandle;}
};

#endif // DC27_WIFI
