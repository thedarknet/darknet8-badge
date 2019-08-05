#ifndef DC27_BLUETOOTH
#define DC27_BLUETOOTH

#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "task.h"

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

	// EXPLOITABLE Stuff
	DN8_IDX_EXPLOITABLE_DATA_RECV_CHAR,
	DN8_IDX_EXPLOITABLE_DATA_RECV_VAL,
	DN8_IDX_EXPLOITABLE_DATA_NOTIFY_CHAR,
	DN8_IDX_EXPLOITABLE_DATA_NOTIFY_VAL,
	DN8_IDX_EXPLOITABLE_DATA_NOTIFY_CFG,

	// GAMEMASTER
	DN8_IDX_GAMEMASTER_DATA_RECV_CHAR,
	DN8_IDX_GAMEMASTER_DATA_RECV_VAL,
	DN8_IDX_GAMEMASTER_DATA_NOTIFY_CHAR,
	DN8_IDX_GAMEMASTER_DATA_NOTIFY_VAL,
	DN8_IDX_GAMEMASTER_DATA_NOTIFY_CFG,

	// BRAINFUZZ
	DN8_IDX_BRAINFUZZ_DATA_RECV_CHAR,
	DN8_IDX_BRAINFUZZ_DATA_RECV_VAL,
	DN8_IDX_BRAINFUZZ_DATA_NOTIFY_CHAR,
	DN8_IDX_BRAINFUZZ_DATA_NOTIFY_VAL,
	DN8_IDX_BRAINFUZZ_DATA_NOTIFY_CFG,
	
	// TTT3D
	DN8_IDX_TTT3D_DATA_RECV_CHAR,
	DN8_IDX_TTT3D_DATA_RECV_VAL,
	DN8_IDX_TTT3D_DATA_NOTIFY_CHAR,
	DN8_IDX_TTT3D_DATA_NOTIFY_VAL,
	DN8_IDX_TTT3D_DATA_NOTIFY_CFG,

	DN8_IDX_NB,
};

#define ESP_GATT_UUID_DN8_SVC 0x444e
static const uint16_t dn8_svc = ESP_GATT_UUID_DN8_SVC;
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;

static const uint16_t serial_service_uuid =   0xABF0;
#define ESP_GATT_UUID_EXPLOITABLE_DATA_RECEIVE     0xABF1
#define ESP_GATT_UUID_EXPLOITABLE_DATA_NOTIFY      0xABF2

#define ESP_GATT_UUID_TTT3D_DATA_RECEIVE        0xDFE0
#define ESP_GATT_UUID_TTT3D_DATA_NOTIFY         0xDFE1

#define ESP_GATT_UUID_BRAINFUZZ_DATA_RECEIVE        0xEFE0
#define ESP_GATT_UUID_BRAINFUZZ_DATA_NOTIFY         0xEFE1

#define ESP_GATT_UUID_GAMEMASTER_DATA_RECEIVE        0xFFE0
#define ESP_GATT_UUID_GAMEMASTER_DATA_NOTIFY         0xFFE1

#define EXPLOITABLE_DATA_MAX_LEN           (512)
#define EXPLOITABLE_CMD_MAX_LEN            (20)
#define EXPLOITABLE_STATUS_MAX_LEN         (20)

#define TTT3D_DATA_MAX_LEN           (512)
#define TTT3D_CMD_MAX_LEN            (20)
#define TTT3D_STATUS_MAX_LEN         (20)

#define BRAINFUZZ_DATA_MAX_LEN           (512)
#define BRAINFUZZ_CMD_MAX_LEN            (20)
#define BRAINFUZZ_STATUS_MAX_LEN         (20)

#define GAMEMASTER_DATA_MAX_LEN           (512)
#define GAMEMASTER_CMD_MAX_LEN            (20)
#define GAMEMASTER_STATUS_MAX_LEN         (20)


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

uint32_t ble_get_passkey(void);
bool ble_get_initialized(void);

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

	BluetoothTask(const std::string &tName, uint16_t stackSize=5000, uint8_t priority=5);
	bool init();
	virtual void run(void* data);
	virtual ~BluetoothTask();

	void setGameTaskQueue(QueueHandle_t queue);
};

#endif // DC27_BLUETOOTH
