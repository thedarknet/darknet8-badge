#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <libesp/task.h>

#include "string.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "ota.h"

char OTA_WIFI_SSID[] = "FIXME\0";
char OTA_WIFI_PASSWORD[] = "FIXME\0";
char OTA_FIRMWARE_UPGRADE_URL[] = "https://192.168.1.170:8070/hello-world.bin\0";

wifi_config_t wifi_config;
esp_http_client_config_t http_config;

const char *OTATask::LOGTAG = "OTATask";
const char *OTA_LOGTAG = "OTATask";

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static EventGroupHandle_t wifi_event_group;


/* Event group allows multiple bits for each event, but we only care
   about one event - are we connected to the API with an IP? */
const int CONNECTED_BIT = BIT0;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	switch(evt->event_id)
	{
	case HTTP_EVENT_ERROR:
		ESP_LOGI(OTA_LOGTAG, "HTTP_EVENT_ERROR");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		ESP_LOGI(OTA_LOGTAG, "HTTP_EVENT_ON_CONNECTED");
		break;
	case HTTP_EVENT_HEADER_SENT:
		ESP_LOGI(OTA_LOGTAG, "HTTP_EVENT_HEADER_SENT");
		break;
	case HTTP_EVENT_ON_HEADER:
		ESP_LOGI(OTA_LOGTAG, "HTTP_EVENT_ON_HEADER");
		break;
	case HTTP_EVENT_ON_DATA:
		ESP_LOGI(OTA_LOGTAG, "HTTP_EVENT_ON_DATA");
		break;
	case HTTP_EVENT_ON_FINISH:
		ESP_LOGI(OTA_LOGTAG, "HTTP_EVENT_ON_FINISH");
		break;
	case HTTP_EVENT_DISCONNECTED:
		ESP_LOGI(OTA_LOGTAG, "HTTP_EVENT_DISCONNECTED");
		break;
	default:
		break;
	}
	return ESP_OK;
}

static void do_ota(void)
{
	esp_err_t ret;

	ESP_LOGI(OTA_LOGTAG, "Starting OTA process...");

	// wait for CONNECTED_BIT to be set in the event group
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(OTA_LOGTAG, "Connected to WiFi network! Attempting to connect to server...");

	http_config.url = OTA_FIRMWARE_UPGRADE_URL;
	http_config.cert_pem = (char*)server_cert_pem_start;
	http_config.event_handler = _http_event_handler;
	ret = esp_https_ota(&http_config);

	if (ret == ESP_OK)
		esp_restart();
	else
		ESP_LOGE(OTA_LOGTAG, "Firmware upgrade failed");

	return;
}

static esp_err_t event_handler(void *ctxt, system_event_t *event)
{
	switch (event->event_id)
	{
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

static void initialize_wifi(void)
{
	// TODO: What if already initialized?
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);

	memcpy(wifi_config.sta.ssid, OTA_WIFI_SSID, strlen(OTA_WIFI_SSID)+1);
	memcpy(wifi_config.sta.password, OTA_WIFI_PASSWORD, strlen(OTA_WIFI_PASSWORD)+1);
	ESP_LOGI(OTA_LOGTAG, "Setting WiFi config SSID %s", wifi_config.sta.ssid);

	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_start();
	return;
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void OTATask::run(void* data)
{
	OTACmd* cmd = nullptr;
	while (1)
	{
		if (xQueueReceive(OTAQueueHandle, &cmd, CmdQueueTimeout))
		{
			switch(*cmd)
			{
			case ATTEMPT_OTA:
				initialize_wifi();
				do_ota();
				break;
			case KILL_OTA:
				// on KILL, clean up the task and die
				goto cleanup;
			default:
				break;
			}
		}
		free(data);
	}
cleanup:
	// TODO: Cleanup?
	ESP_LOGI(LOGTAG, "COMPLETE");
	return;	
}

bool OTATask::init()
{
	this->OTAQueueHandle = xQueueCreateStatic(OTA_QUEUE_SIZE, OTA_MSG_SIZE,
		otaQueueBuffer, &OTAQueue);
	return true;
}

OTATask::OTATask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = OTA_LOGTAG;
	ESP_LOGI(LOGTAG, "CREATED");
	// TODO
	return;
}


OTATask::~OTATask()
{
	ESP_LOGI(LOGTAG, "DESTROY");
	// TODO Cleanup?
	return;
}
