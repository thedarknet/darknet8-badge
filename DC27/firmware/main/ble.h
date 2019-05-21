#ifndef DC27_BLUETOOTH
#define DC27_BLUETOOTH

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <libesp/task.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#include "./game_master.h"

enum
{
	// Main service
	DN8_IDX_SVC,

	// SERIAL Stuff
	DN8_IDX_SERIAL_DATA_RECV_CHAR,
	DN8_IDX_SERIAL_DATA_RECV_VAL,
	DN8_IDX_SERIAL_DATA_NOTIFY_CHAR,
	DN8_IDX_SERIAL_DATA_NOTIFY_VAL,
	DN8_IDX_SERIAL_DATA_NOTIFY_CFG,
	DN8_IDX_SERIAL_COMMAND_CHAR,
	DN8_IDX_SERIAL_COMMAND_VAL,
	DN8_IDX_SERIAL_STATUS_CHAR,
	DN8_IDX_SERIAL_STATUS_VAL,
	DN8_IDX_SERIAL_STATUS_CFG,
#ifdef SUPPORT_HEARTBEAT
	//DN8_IDX_SERIAL_HEARTBEAT_CHAR,
	//DN8_IDX_SERIAL_HEARTBEAT_VAL,
	//DN8_IDX_SERIAL_HEARTBEAT_CFG,
#endif // SUPPORT_HEARTBEAT

	DN8_IDX_NB,
};


#define ESP_GATT_UUID_DN8_SVC 0x444e
static const uint16_t dn8_svc = ESP_GATT_UUID_DN8_SVC;
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;

static const uint16_t serial_service_uuid =   0xABF0;
#define ESP_GATT_UUID_SERIAL_DATA_RECEIVE     0xABF1
#define ESP_GATT_UUID_SERIAL_DATA_NOTIFY      0xABF2
#define ESP_GATT_UUID_SERIAL_COMMAND_RECEIVE  0xABF3
#define ESP_GATT_UUID_SERIAL_COMMAND_NOTIFY   0xABF4
#ifdef SUPPORT_HEARTBEAT
#define ESP_GATT_UUID_SERIAL_HEARTBEAT        0xABF5
#endif

#define SERIAL_DATA_MAX_LEN           (512)
#define SERIAL_CMD_MAX_LEN            (20)
#define SERIAL_STATUS_MAX_LEN         (20)

struct gatts_profile_inst
{
	esp_gatts_cb_t gatts_cb;
	uint16_t gatts_if;
	uint16_t app_id;
	uint16_t conn_id;
	uint16_t service_handle;
	esp_gatt_srvc_id_t service_id;
	uint16_t char_handle;
	esp_bt_uuid_t char_uuid;
	esp_gatt_perm_t perm;
	esp_gatt_char_prop_t property;
	uint16_t descr_handle;
	esp_bt_uuid_t descr_uuid;
};

typedef struct serial_recv_data_node
{
	int32_t len;
	uint8_t * node_buff;
	struct serial_recv_data_node * next_node;
} serial_recv_data_node_t;

typedef struct serial_recv_data_buff
{
	int32_t node_num;
	int32_t buff_size;
	serial_recv_data_node_t * first_node;
} serial_recv_data_buff_t;

class BluetoothTask : public Task {
private:
	QueueHandle_t gameTaskQueue = nullptr;

	static const int BLE_QUEUE_SIZE = 3;
	static const int BLE_MSG_SIZE = sizeof(GameMsg);
	StaticQueue_t BLEGameQueue;
	QueueHandle_t BLEGameQueueHandle = nullptr;
	uint8_t bleGameQueueBuffer[BLE_QUEUE_SIZE * BLE_MSG_SIZE];
	void gameCommandHandler(GameMsg* msg);

	// TODO: General Command Queue

public:
	static const char *LOGTAG;

	BluetoothTask(const std::string &tName, uint16_t stackSize=10000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~BluetoothTask();

	void setGameTaskQueue(QueueHandle_t queue);
};

#endif // DC27_BLUETOOTH
