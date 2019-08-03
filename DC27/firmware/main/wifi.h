#ifndef DC27_WIFI
#define DC27_WIFI

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "task.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <list>

#include "wifi/WiFi.h"

enum WIFIResponseType {
	WIFI_OK            = 0x00, // Request 
	WIFI_SCAN_RESP     = 0x01, // Found someone, returning data
	WIFI_SCAN_COMPLETE = 0x02, // No longer scanning

	// WIFI_ERR_x = 0x8_,
	WIFI_ERR_CONNECTAP = 0x80,
	WIFI_ERR_UNK = 0xFF
};

struct ScanResult {
	char bssid[6];
	char ssid[33];
	wifi_auth_mode_t authmode;
};

struct WiFiScanResult {
	WiFiScanResult() : Length(0), ResultArray(0) {}
	uint8_t	Length;
	ScanResult *ResultArray;
};

struct WIFIResponseMsg {
	WIFIResponseMsg(const WIFIResponseType &t) : type(t), SRes() {}
	~WIFIResponseMsg() {
		std::list<ScanResult*>::iterator it = SRes.begin();
		while(it!=SRes.end()) {
			delete (*it);
			++it;
		}
		SRes.clear();
	}

	WIFIResponseType type;
	std::list<ScanResult*> SRes;
};

enum WIFICmd {
	WIFI_ATTEMPT_OTA  = 0x00,
	WIFI_AP_START     = 0x01,
	WIFI_AP_STOP      = 0x02,
	WIFI_SCAN         = 0x03,
	WIFI_NPC_INTERACT = 0x04,
};

enum NpcMsgType {
	NPC_LIST   = 0x00,
	NPC_ACTION = 0x01
};

struct WifiNpcMsg {
	NpcMsgType type;
	char* npcname;
	char* ssid;
	uint8_t* data;
};

struct WifiScanMsg {
	bool npcfilter;
};

struct StartAPMsg {
	wifi_auth_mode_t mode;
	char* ssid;
	char* passwd;
};

struct WIFIMsg {
	WIFICmd cmd;
	union {
		WifiNpcMsg wnm;
		WifiScanMsg wsm;
		StartAPMsg sap;
	} data;
	QueueHandle_t returnQueue;
};

class WIFITask : public Task {
public:
	static const int WIFI_QUEUE_SIZE = 3;
	static const int WIFI_MSG_SIZE = sizeof(WIFIMsg);
	static const char *LOGTAG;
public:
	WIFITask(const std::string &tName, uint16_t stackSize=5000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~WIFITask();
	QueueHandle_t getQueueHandle() {return WIFIQueueHandle;}
private:
	QueueHandle_t WIFIQueueHandle = nullptr;
	uint32_t my_nvs_handle;

	void wifiScan(WIFIMsg* msg);
	void npcInteract(WIFIMsg* msg);
	void startAp(WIFIMsg* msg);
	void stopAp(WIFIMsg* msg);
protected:
	WiFi wifi;
};

#endif // DC27_WIFI
