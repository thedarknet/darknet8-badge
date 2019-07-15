#include "ota_menu.h"
#include "menu_state.h"
#include "gui_list_processor.h"
#include "../app.h"
#include "../buttons.h"
#include <libesp/system.h>

#include <esp_log.h>
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


using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;

const char* OTA_LOGTAG = "OTAMenu";

char OTA_WIFI_SSID[] = "FIXME\0";
char OTA_WIFI_PASSWORD[] = "FIXME\0";
char OTA_FIRMWARE_UPGRADE_URL[] = "https://192.168.1.170:8070/dc27.bin\0";

wifi_config_t wifi_config;
esp_http_client_config_t http_config;
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



void OTAMenu::do_factory_reset(void)
{
    const esp_partition_t* esp_part = NULL;
    esp_partition_iterator_t itr;

    itr = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (itr == NULL)
    {
        ESP_LOGE(OTA_LOGTAG, "Failed to find factory partition");
    }
    esp_part = esp_partition_get(itr);
    esp_ota_set_boot_partition(esp_part);
    nvs_set_i32(my_nvs_handle, "attempted_boot", 0);
    nvs_set_i32(my_nvs_handle, "boot_successful", 0);
    nvs_commit(my_nvs_handle);
    esp_restart();
}


OTAMenu::OTAMenu() : DN8BaseMenu(), 
	OTAList("Badge Info:", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0, (sizeof(Items) / sizeof(Items[0])))
{
}

OTAMenu::~OTAMenu() {

}

ErrorType OTAMenu::onInit() {
	clearListBuffer();
	sprintf(getRow(0), "Attempting OTA update");
	for (uint32_t i = 0; i < (sizeof(Items) / sizeof(Items[0])); i++) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawList(&OTAList);

    nvs_open("storage", NVS_READWRITE, &this->my_nvs_handle);

	return ErrorType();
}

BaseMenu::ReturnStateContext OTAMenu::onRun() {

	BaseMenu *nextState = this;
	if (DN8App::get().getButtonInfo().wereTheseButtonsReleased(
		ButtonInfo::BUTTON_RIGHT_DOWN | ButtonInfo::BUTTON_LEFT_UP))
	{
		nextState = DN8App::get().getMenuState();
		if (nextState == (void*)0x12345678)
		{
			// I don't quite understand the state thing yet (again)
			// so i'm just doing garbage here as a placeholder
			initialize_wifi();
			do_ota();
		}
	} else if(GUIListProcessor::process(&OTAList,OTAList.ItemsCount)) {
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
		DN8App::get().getGUI().drawList(&OTAList);
	}
	return ReturnStateContext(nextState);
}

ErrorType OTAMenu::onShutdown() {
	return ErrorType();
}

