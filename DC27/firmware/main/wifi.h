#ifndef DC27_WIFI
#define DC27_WIFI

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "task.h"
#include <nvs_flash.h>
#include <nvs.h>
#include "wifi/WiFi.h"
#include <error_type.h>


enum WIFIResponseType {
	WIFI_OK            = 0x00, // Request 
	WIFI_SCAN_RESP     = 0x01, // Found someone, returning data
	WIFI_SCAN_COMPLETE = 0x02, // No longer scanning
	WIFI_STATUS_OK		 = 0x03,

	WIFI_OTA_NOT_START = 0x10,
	WIFI_OTA_START     = 0x11,
	WIFI_OTA_CONNECT   = 0x12,
	WIFI_OTA_DOWNLOAD  = 0x13,
	WIFI_OTA_REBOOT    = 0x14,

	// WIFI_ERR_x = 0x8_,
	WIFI_ERR_CONNECTAP = 0x80,
	WIFI_ERR_OTA       = 0x81,
	WIFI_ERR_UNK       = 0xFF
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

class StatusMsg {
public:
	StatusMsg(bool isApStarted ) : IsApStarted(isApStarted) {
		memset(&SSID[0],0,sizeof(SSID));
		memset(&Password[0],0,sizeof(Password));
		memset(&IPAddress[0],0,sizeof(IPAddress));
	}
	bool isApStarted() const {return IsApStarted;}
	void setSSID(const char *s) {
		strncpy(&SSID[0],s,sizeof(SSID));
	}
	void setPassword(const char *p) {
		strncpy(&Password[0],p,sizeof(Password));
	}
	void setIP(const char *ip) {
		strncpy(&IPAddress[0],ip,sizeof(IPAddress));
	}
	const char *getSSID() const {return &SSID[0];}
	const char *getIP() const {return &IPAddress[0];}
private:
	bool IsApStarted;
	char SSID[17];
	char Password[30];
	char IPAddress[16];
};


enum WIFICmd {
	WIFI_ATTEMPT_OTA  = 0x00,
	WIFI_AP_START     = 0x01,
	WIFI_AP_STOP      = 0x02,
	WIFI_SCAN         = 0x03,
	WIFI_NPC_INTERACT = 0x04,
	WIFI_STATUS			= 0x05,
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
	char SSID[17];
	char Password[30];
};


class WIFIRequestMsg {
public:
	WIFIRequestMsg(WIFICmd type, QueueHandle_t &t) : cmd(type),data(),returnQueue(t) {}
	~WIFIRequestMsg() {
		switch(cmd) {
			case WIFI_AP_START:
				delete data.sap;
			break;
			case WIFI_SCAN:
				delete data.wsm;
			break;
			case WIFI_NPC_INTERACT:
				delete data.wnm;
			default:
			break;
		}
	}
	void setStartAPMsg(StartAPMsg *msg) {data.sap = msg;}
public:
	WIFICmd cmd;
	union {
		WifiNpcMsg  *wnm;
		WifiScanMsg *wsm;
		StartAPMsg  *sap;
	} data;
	QueueHandle_t returnQueue;
};

class WIFIResponseMsg {
public:
	WIFIResponseMsg(const WIFIResponseType &t) : type(t), data() {
		data.wsr = 0;
	}
	~WIFIResponseMsg() { 
		switch(type) {
			case WIFI_OK:
			break;
			case WIFI_SCAN_RESP:
			break;
			case WIFI_STATUS_OK:
				delete data.smsg;
			break;
			case WIFI_SCAN_COMPLETE:
			break;
			default:
			break;
		}
	}
	void setResponse(StatusMsg *s) {data.smsg = s;}
	WIFIResponseType &getType() {return type;}
	const StatusMsg &getStatusMsg() {return *data.smsg;}
private:
	WIFIResponseType type;
	union {
		WiFiScanResult *wsr;
		StatusMsg	*smsg;
	} data;
};

enum WIFIResponseType wifi_get_ota_status(void);
class WIFITask : public Task {
public:
	static const int WIFI_QUEUE_SIZE = 3;
	static const int WIFI_MSG_SIZE = sizeof(WIFIRequestMsg);
	static const char *LOGTAG;
	enum WIFIResponseType OTAStatus = WIFI_OTA_NOT_START;
public:
	WIFITask(const std::string &tName, uint16_t stackSize=5000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~WIFITask();
	QueueHandle_t getQueueHandle() {return WIFIQueueHandle;}
	libesp::ErrorType requestStatus(QueueHandle_t &t);
	libesp::ErrorType requestAPDown(QueueHandle_t &t);
	libesp::ErrorType requestAPUp(QueueHandle_t &t, uint16_t secType,const char *ssid,const char *pw);
	libesp::ErrorType requestOTA(QueueHandle_t &t);
	enum WIFIResponseType getOTAStatus(void);
private:
	QueueHandle_t WIFIQueueHandle = nullptr;
	uint32_t my_nvs_handle;

	void wifiScan(WIFIRequestMsg* msg);
	void npcInteract(WIFIRequestMsg* msg);
	void startAp(WIFIRequestMsg* msg);
	void stopAp(WIFIRequestMsg* msg);
	void handleStatus(WIFIRequestMsg*);
protected:
	WiFi wifi;
};

#endif // DC27_WIFI
