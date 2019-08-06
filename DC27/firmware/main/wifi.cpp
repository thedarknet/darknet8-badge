#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "task.h"
#include <nvs.h>
#include <nvs_flash.h>

#include "string.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"

#include "factory_reset.h"
#include "./wifi.h"
#include "./app.h"
#include "npc_interact.h"

#include "wifi/WiFi.h"
#include "HttpServer.h"
#include "system.h"

const char *WIFITask::LOGTAG = "WIFITask";
static const char *LOGTAG = "OTAWIFI";

/**************************** OTA CODE **********************************/
char OTA_WIFI_SSID[] = "DN8OTA\0";
char OTA_WIFI_PASSWORD[] = "DN8OTAPW\0";
char OTA_FIRMWARE_UPGRADE_URL[] = "https://192.168.4.1:4433/dc27.bin\0";
wifi_config_t wifi_config;
esp_http_client_config_t http_config;
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
static EventGroupHandle_t wifi_event_group;
/* Event group allows multiple bits for each event, but we only care
   about one event - are we connected to the API with an IP? */
const int CONNECTED_BIT = BIT0;

static StaticQueue_t WIFIQueue;
static uint8_t otaQueueBuffer[WIFITask::WIFI_QUEUE_SIZE * WIFITask::WIFI_MSG_SIZE];

static esp_err_t ota_http_event_handler(esp_http_client_event_t *evt) {
	switch(evt->event_id) {
	case HTTP_EVENT_ERROR:
		ESP_LOGI(LOGTAG, "HTTP_EVENT_ERROR");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		ESP_LOGI(LOGTAG, "HTTP_EVENT_ON_CONNECTED");
		break;
	case HTTP_EVENT_HEADER_SENT:
		ESP_LOGI(LOGTAG, "HTTP_EVENT_HEADER_SENT");
		break;
	case HTTP_EVENT_ON_HEADER:
		ESP_LOGI(LOGTAG, "HTTP_EVENT_ON_HEADER");
		break;
	case HTTP_EVENT_ON_DATA:
		ESP_LOGI(LOGTAG, "HTTP_EVENT_ON_DATA");
		break;
	case HTTP_EVENT_ON_FINISH:
		ESP_LOGI(LOGTAG, "HTTP_EVENT_ON_FINISH");
		break;
	case HTTP_EVENT_DISCONNECTED:
		ESP_LOGI(LOGTAG, "HTTP_EVENT_DISCONNECTED");
		break;
	default:
		break;
	}
	return ESP_OK;
}


static enum WIFIResponseType OTAStatus;
static void do_ota(void) {
	esp_err_t ret;

	ESP_LOGI(LOGTAG, "Starting OTA process...");

	// wait for CONNECTED_BIT to be set in the event group
	OTAStatus = WIFI_OTA_CONNECT;
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(LOGTAG, "Connected to WiFi network! Attempting to connect to server...");


	OTAStatus = WIFI_OTA_DOWNLOAD;
	http_config.url = OTA_FIRMWARE_UPGRADE_URL;
	http_config.cert_pem = (char*)server_cert_pem_start;
	http_config.event_handler = ota_http_event_handler;
	ret = esp_https_ota(&http_config);

	if (ret == ESP_OK)
	{
		OTAStatus = WIFI_OTA_REBOOT;
		esp_restart();
	}
	else
	{
		OTAStatus = WIFI_ERR_OTA;
		ESP_LOGE(LOGTAG, "Firmware upgrade failed");
	}

	return;
}

static esp_err_t ota_event_handler(void *ctxt, system_event_t *event) {
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		// Workaround as ESP32 Wifi libs don't current re-associate
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

static void initialize_wifi(void) {
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK( esp_event_loop_init(ota_event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);

	memcpy(wifi_config.sta.ssid, OTA_WIFI_SSID, strlen(OTA_WIFI_SSID)+1);
	memcpy(wifi_config.sta.password, OTA_WIFI_PASSWORD, strlen(OTA_WIFI_PASSWORD)+1);
	ESP_LOGI(LOGTAG, "Setting WiFi config SSID %s", wifi_config.sta.ssid);

	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_start();
	return;
}

/**************************** END CODE **********************************/


/************************** DC27 WIFI CODE ******************************/
//#include "npc_interact.h"
static NPCInteractionTask NPCITask("NPCInteractTask");
static libesp::HttpServer Port80WebServer;

class MyWiFiEventHandler: public WiFiEventHandler {
public:
	const char *logTag = "MyWiFiEventHandler";
	MyWiFiEventHandler() : APStarted(false), scanMsgID(0), NPCOnlyFilter(false) {}

	virtual esp_err_t staGotIp(system_event_sta_got_ip_t event_sta_got_ip) {
		ESP_LOGD(logTag, "MyWiFiEventHandler(Class): staGotIp");
		return ESP_OK;
	}   
	virtual esp_err_t apStart() {
		ESP_LOGI(logTag, "MyWiFiEventHandler(Class): apStart");
		APStarted = true;
		libesp::System::get().logSystemInfo();
		ESP_LOGI(logTag, "Starting Web Server");
		Port80WebServer.start(80, false);
		ESP_LOGI(logTag, "after starting Web server");
		libesp::System::get().logSystemInfo();
		return ESP_OK;
	}   
	virtual esp_err_t apStop() {
		ESP_LOGI(logTag, "MyWiFiEventHandler(Class): apStop stop");
		APStarted = false;
		return ESP_OK;
	}   
	virtual esp_err_t wifiReady() {
		ESP_LOGI(logTag, "MyWiFiEventHandler(Class): wifi ready");
		return ESP_OK;
	}   
	virtual esp_err_t staConnected(system_event_sta_connected_t info) {
		std::string s((const char *)&info.ssid[0],info.ssid_len);
		ESP_LOGI(logTag,"Connected to: %s on channel %d",s.c_str(), (int)info.channel);
		return ESP_OK;
	}   
	virtual esp_err_t staDisconnected(system_event_sta_disconnected_t info) {
		return ESP_OK;
	}   
	virtual esp_err_t staScanDone(system_event_sta_scan_done_t info) {
			  /*
		ESP_LOGI(logTag, "MyWiFiEventHandler(Class): scan done: APs Found %d", (int32_t) info.number);
		uint16_t numAPs = info.number;
		wifi_ap_record_t *recs = new wifi_ap_record_t[numAPs];
		int numberToReturn = 0;
		WIFIResponseMsg* rmsg = new WIFIResponseMsg(WIFI_SCAN_COMPLETE);
		if(ESP_OK==esp_wifi_scan_get_ap_records(&numAPs,recs)) {
			for(auto i=0;i<numAPs;++i) {
				if(numberToReturn<5) {
					if(strstr("dark",(const char *)recs[i].ssid)
						&& recs[i].bssid[0]==0xdc
						&& recs[i].bssid[1]==0xd0) {

						ScanResult* sres = new ScanResult();
						memcpy(sres->bssid, recs[i].bssid, 6);
						memcpy(sres->ssid, recs[i].ssid, 33);
						sres->authmode = recs[i].authmode;
						rmsg->SRes.push_back(sres);

						++numberToReturn;
					}
				}
			}
		}
		xQueueSend(respQueue, (void* )&rmsg,(TickType_t ) 100);
		delete [] recs;
		*/
		return ESP_OK;
	}
	virtual esp_err_t staAuthChange(system_event_sta_authmode_change_t info) { return ESP_OK; }
	virtual esp_err_t staStart() { return ESP_OK; }
	virtual esp_err_t staStop() { return ESP_OK; }
	bool isAPStarted() {return APStarted;}
	uint16_t getScanMsgID() {return scanMsgID;}
	void setScanMsgID(uint16_t sid) {scanMsgID=sid;}
	void setWiFiScanNPCOnly(bool b) { NPCOnlyFilter = b; }
	void setWiFiScanResponseQueue(QueueHandle_t rq) { respQueue = rq; }
private:
	bool APStarted;
	uint16_t scanMsgID;
	bool NPCOnlyFilter;
	QueueHandle_t respQueue;
};


void WIFITask::npcInteract(WIFIRequestMsg* msg) {
	bool success = false;
	WifiNpcMsg* wnm = msg->data.wnm;
	ESP_LOGI(LOGTAG, "processing wifi npc interaction");
	wifi.stopWiFi();
	if (ESP_OK == wifi.connectAP(wnm->ssid, (const char*)"DCDN-8-DC27", wnm->data)) {
		NPCInteractionTask::NPCMsg* nmsg = nullptr;
		if (wnm->type == NPC_LIST) { // NPC List Request
			nmsg = new NPCInteractionTask::NPCMsg(NPCInteractionTask::NPCMsg::HELO,
				msg->returnQueue); // msgid?
		} else { // NPC Interaction Request
			nmsg = new NPCInteractionTask::NPCMsg(NPCInteractionTask::NPCMsg::INTERACT,
				msg->returnQueue, wnm->npcname, 0);
		}
		xQueueSend(NPCITask.getQueueHandle(), (void* )&nmsg,(TickType_t ) 100);
		success = true;
	}

	free(wnm->ssid);
	free(wnm->data);
	if (!success) free(wnm->npcname); // it not successful, this wasn't passed anywhere

	// Send an OK or error response
	WIFIResponseMsg* resp = new WIFIResponseMsg(success ? WIFI_OK : WIFI_ERR_CONNECTAP);
	xQueueSend(msg->returnQueue, (void*)&resp, (TickType_t)100);
}



void WIFITask::wifiScan(WIFIRequestMsg* msg)
{
	WifiScanMsg* wsm = msg->data.wsm;	
	MyWiFiEventHandler *eh = (MyWiFiEventHandler*)wifi.getWifiEventHandler();
	ESP_LOGI(LOGTAG, "processing wifi scan");
	if (eh) {
		eh->setWiFiScanResponseQueue(msg->returnQueue);
		eh->setWiFiScanNPCOnly(wsm->npcfilter);
		wifi.stopWiFi();
		ESP_LOGI(LOGTAG, "starting scan request");
		wifi.scan(false);
	}
	return;
}

void WIFITask::startAp(WIFIRequestMsg* msg) {
	StartAPMsg* sap = msg->data.sap;

	// Start the AP and the webserver
	wifi.startAP(&sap->SSID[0], &sap->Password[0], sap->mode);

	if(msg->returnQueue) {
		// Send an OK response
		WIFIResponseMsg* resp = new WIFIResponseMsg(WIFI_OK);
		xQueueSend(msg->returnQueue, (void*)&resp, (TickType_t)10);
	}
}

void WIFITask::stopAp(WIFIRequestMsg* msg) {
	// Shut down webserver and AP
	Port80WebServer.stop();
	wifi.stopWiFi();
	wifi.shutdown();

	if(msg->returnQueue) {
		// Send an OK response
		WIFIResponseMsg* resp = new WIFIResponseMsg(WIFI_OK);
		xQueueSend(msg->returnQueue, (void*)&resp, (TickType_t)10);
	}
}

void WIFITask::handleStatus(WIFIRequestMsg *msg) {
	WIFIResponseMsg *resp = new WIFIResponseMsg(WIFI_STATUS_OK);
	MyWiFiEventHandler *eh = (MyWiFiEventHandler*)wifi.getWifiEventHandler();
	StatusMsg *smg = new StatusMsg(eh->isAPStarted());
	resp->setResponse(smg);
	smg->setSSID(wifi.getApSSID().c_str());
	smg->setPassword("");
	smg->setIP(wifi.getApIp().c_str());
	wifi.dump();
	xQueueSend(msg->returnQueue, (void*)&resp, (TickType_t)10);
}
	
libesp::ErrorType WIFITask::requestStatus(QueueHandle_t &t) {
	WIFIRequestMsg *req = new WIFIRequestMsg(WIFI_STATUS, t);
	return xQueueSend(getQueueHandle(),&req, (TickType_t)10);
}
	
libesp::ErrorType WIFITask::requestAPDown(QueueHandle_t &t) {
	WIFIRequestMsg *req = new WIFIRequestMsg(WIFI_AP_STOP, t);
	return xQueueSend(getQueueHandle(),&req, (TickType_t)10);
}

libesp::ErrorType WIFITask::requestAPUp(QueueHandle_t &t, uint16_t secType,const char *ssid,const char *pw) {
	WIFIRequestMsg *req = new WIFIRequestMsg(WIFI_AP_START, t);
	StartAPMsg *sapmsg = new StartAPMsg();
	sapmsg->mode = (wifi_auth_mode_t)secType;
	strncpy(&sapmsg->SSID[0],ssid,sizeof(sapmsg->SSID));
	strncpy(&sapmsg->Password[0],pw,sizeof(sapmsg->Password));
	req->setStartAPMsg(sapmsg);
	return xQueueSend(getQueueHandle(),&req, (TickType_t)10);
}

libesp::ErrorType WIFITask::requestOTA(QueueHandle_t &t)
{
	OTAStatus = WIFI_OTA_NOT_START;
	WIFIRequestMsg *req = new WIFIRequestMsg(WIFI_ATTEMPT_OTA,
		t);
	return xQueueSend(getQueueHandle(), &req, (TickType_t)10);
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
#define BOOT_CONSIDERED_SUCCESS (30)
void WIFITask::run(void* data) {
	WIFIRequestMsg* msg = nullptr;
	int since_boot = 0;
	while (1) {
		if (xQueueReceive(WIFIQueueHandle, &msg, CmdQueueTimeout)) {
			switch(msg->cmd) {
			case WIFI_ATTEMPT_OTA:
				ESP_LOGI(LOGTAG, "Starting OTA");
				OTAStatus = WIFI_OTA_START;
				wifi.stopWiFi();
				wifi.shutdown();
				initialize_wifi();
				do_ota();
				break;
			case WIFI_AP_START:
				startAp(msg);
				break;
			case WIFI_AP_STOP:
				stopAp(msg);
				break;
			case WIFI_SCAN:
				wifiScan(msg);
				break;
			case WIFI_NPC_INTERACT:
				npcInteract(msg);
				break;
			case WIFI_STATUS:
				handleStatus(msg);
			default:
				break;
			}
			delete msg;
		} else if (since_boot >= 0) { // on timeout (1 second?)
			if (since_boot > BOOT_CONSIDERED_SUCCESS) {
				ESP_LOGI(LOGTAG, "Boot considered successful");
				nvs_set_i32(this->my_nvs_handle, "attempted_boot", 0);
				nvs_set_i32(this->my_nvs_handle, "boot_successful", 0);
				nvs_commit(this->my_nvs_handle);
				since_boot = -1; // never enter this else case again
			} else {
				since_boot++; // increment and move on
			}
		}
	}
	ESP_LOGI(LOGTAG, "COMPLETE");
	return;	
}

enum WIFIResponseType WIFITask::getOTAStatus(void)
{
	return OTAStatus;
}

bool WIFITask::init() {
	int32_t attempted_boot = 0;
	int32_t boot_successful = 0;
	esp_err_t err;
	err = nvs_open("nvs", NVS_READWRITE, &this->my_nvs_handle);
	if (err != ESP_OK) {
		ESP_LOGE(LOGTAG, "FAILED TO OPEN NVS");
		return false;
	}

	nvs_get_i32(this->my_nvs_handle, "attempted_boot", &attempted_boot);
	nvs_get_i32(this->my_nvs_handle, "boot_successful", &boot_successful);
	if ((attempted_boot - boot_successful) > 6) {
		ESP_LOGE(LOGTAG, "Factory Reset Requested (or device crashed more than 10 times");
		do_factory_reset();
		// if we didn't reboot, reset the boot counters
		nvs_set_i32(this->my_nvs_handle, "attempted_boot", 0);
		nvs_set_i32(this->my_nvs_handle, "boot_successful", 0);
	}
	attempted_boot++;
	err = nvs_set_i32(this->my_nvs_handle, "attempted_boot", attempted_boot);
	nvs_commit(this->my_nvs_handle);

	this->WIFIQueueHandle = xQueueCreateStatic(WIFI_QUEUE_SIZE, WIFI_MSG_SIZE,
		otaQueueBuffer, &WIFIQueue);

	NPCITask.init();
	NPCITask.start();
	WiFiEventHandler* handler = new MyWiFiEventHandler();
	wifi.setWifiEventHandler(handler);
	return true;
}

WIFITask::WIFITask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority) {
	ESP_LOGI(LOGTAG, "CREATED");
	OTAStatus = WIFI_OTA_NOT_START;
	return;
}


WIFITask::~WIFITask() {
	ESP_LOGI(LOGTAG, "DESTROY");
	NPCITask.stop();
	return;
}
