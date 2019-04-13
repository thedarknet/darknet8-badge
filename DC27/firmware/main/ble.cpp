
#include "ble.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <map>
#include <nvs_flash.h>

// bluetooth libraries
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

BluetoothTask *pBTTask;
const char *BluetoothTask::LOGTAG = "BluetoothTask";
const char *LOGTAG = "BluetoothService";

#define DN8_PROFILE_NUM       0x1
#define DN8_PROFILE_APP_IDX   0x0
#define DN8_APP_ID            0x55
#define DN8_DEVICE_NAME       "DN8_SECURITY"
#define DN8_SVC_INST_ID       0x0

uint16_t dn8_handle_tbl[DN8_IDX_NB];

/* Scan and Advertisement Configuration */
#define ADV_CONFIG_FLAG       (1 << 0)
#define SCAN_RSP_CONFIG_FLAG  (1 << 1)
static uint8_t dn8_manufacturer[3];
static uint8_t sec_service_uuid[16];
static uint8_t adv_config_done = 0;
static esp_ble_adv_data_t dn8_adv_config;
static esp_ble_adv_params_t dn8_adv_params;
static esp_ble_adv_data_t dn8_scan_rsp_config;

#define ESP_GATT_UUID_DN8_SVC 0x444e
static const uint16_t dn8_svc = ESP_GATT_UUID_DN8_SVC;
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;


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
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
	esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);

static struct gatts_profile_inst dn8_profile_tab[DN8_PROFILE_NUM];


#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void BluetoothTask::run(void* data)
{
	ESP_LOGI(LOGTAG, "RUNNING");
	while (1)
	{
		//ESP_LOGI(LOGTAG, "BOOP");
		vTaskDelay(CmdQueueTimeout);
	}
	ESP_LOGI(LOGTAG, "COMPLETE");
}


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	esp_err_t ret;
	ESP_LOGV(LOGTAG, "GAP_EVT, event %d\n", event);

	switch (event)
	{
		// Security Server Example
		case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
			// When we've complete a scan, turn advertising back on
			adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
			if (adv_config_done == 0)
				esp_ble_gap_start_advertising(&dn8_adv_params);
			break;
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
			// When we've finished setting advertising data, turn advertising back on
			adv_config_done &= (~ADV_CONFIG_FLAG);
			if (adv_config_done == 0)
				esp_ble_gap_start_advertising(&dn8_adv_params);
			break;
		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
			// Fires after advertising is turned on to indicate success or failure
			if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
				ESP_LOGE(LOGTAG, "Advertising Start Failed: %x", param->adv_start_cmpl.status);
			else
				ESP_LOGI(LOGTAG, "Advertising Start Succeess");
			break;
		case ESP_GAP_BLE_PASSKEY_REQ_EVT:
			// Passkey request event, in the case of a non-preset passkey
			// TODO: This, we will want to replace a preset passkey
			ESP_LOGI(LOGTAG, "PASSKEY_REQ_EVT");
			break;
		case ESP_GAP_BLE_SEC_REQ_EVT:
			// Respond to security request with the affirmative to start the handshake
			esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
			break;
		case ESP_GAP_BLE_NC_REQ_EVT:
			// Received When IO has Display Yes/No and Peer has Display Yes/No capability
			// show the passkey number to the user to confirm it with the number displayed
			// TODO: print to screen
			ESP_LOGI(LOGTAG, "NC_REQ_EVT, Passkey: %d", param->ble_security.key_notif.passkey);
			break;
		case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
			// Received when IO has Output and peer has Input capability
			ESP_LOGI(LOGTAG, "NOTIF_EVT, Passkey: %06d", param->ble_security.key_notif.passkey);
			break;
		case ESP_GAP_BLE_KEY_EVT:
			// Shows BLE key info shared with perr device to user
			ESP_LOGI(LOGTAG, "KEY_EVT, Key Type: 0x%x", param->ble_security.ble_key.key_type);
			break;
		case ESP_GAP_BLE_AUTH_CMPL_EVT:
			if (param->ble_security.auth_cmpl.success)
				ESP_LOGI(LOGTAG, "CMPL, Mode: 0x%x", param->ble_security.auth_cmpl.auth_mode);
			else
				ESP_LOGE(LOGTAG, "CMPL, Fail: 0x%x", param->ble_security.auth_cmpl.fail_reason);
			break;
		case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
			ESP_LOGI(LOGTAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status: %d",
				param->remove_bond_dev_cmpl.status);
			break;
		case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
			ESP_LOGI(LOGTAG, "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT");
			if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS)
			{
				ESP_LOGE(LOGTAG, "Config Local Privacy Failed: %x",
					param->local_privacy_cmpl.status);
				break;
			}
			ret = esp_ble_gap_config_adv_data(&dn8_adv_config);
			if (ret)
				ESP_LOGE(LOGTAG, "config adv data failed, error code = %x", ret);
			else
				adv_config_done |= ADV_CONFIG_FLAG;

			ret = esp_ble_gap_config_adv_data(&dn8_scan_rsp_config);
			if (ret)
				ESP_LOGE(LOGTAG, "config adv data failed, error code = %x", ret);
			else
				adv_config_done |= SCAN_RSP_CONFIG_FLAG;

			break;
		case ESP_GAP_BLE_OOB_REQ_EVT: // unused
		case ESP_GAP_BLE_LOCAL_IR_EVT: // unused
		case ESP_GAP_BLE_LOCAL_ER_EVT: // unused
		default:
			ESP_LOGI(LOGTAG, "Unused GAP_BLE_.*_EVT");
			break;
	}
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
	esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	ESP_LOGV(LOGTAG, "GATTS Profile Event = %x\n", event);
	switch (event)
	{
		case ESP_GATTS_REG_EVT:
			esp_ble_gap_set_device_name(DN8_DEVICE_NAME);
			esp_ble_gap_config_local_privacy(true);
			//esp_ble_gatts_create_attr_tab(dn8_gatt_db, gatts_if,
			//	DN8_IDX_NB, DN8_SVC_INST_ID);
			break;
		case ESP_GATTS_CONNECT_EVT:
			// setup encryption
			ESP_LOGI(LOGTAG, "ESP_GATTS_CONNECT_EVT");
			esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
			break;
		case ESP_GATTS_DISCONNECT_EVT:
			// Start advertising again
			ESP_LOGI(LOGTAG, "ESP_GATTS_DISCONNECT_EVT");
			esp_ble_gap_start_advertising(&dn8_adv_params);
			break;
		case ESP_GATTS_CREAT_ATTR_TAB_EVT:
			ESP_LOGI(LOGTAG, "The number handle = %x",param->add_attr_tab.num_handle);
			if (param->create.status == ESP_GATT_OK)
			{
				if(param->add_attr_tab.num_handle == DN8_IDX_NB)
				{
					memcpy(dn8_handle_tbl, param->add_attr_tab.handles, sizeof(dn8_handle_tbl));
					esp_ble_gatts_start_service(dn8_handle_tbl[DN8_IDX_SVC]);
				}
				else
					ESP_LOGE(LOGTAG, "Create attribute table event abnormal - Bad Handle Num");
			}
			else
				ESP_LOGE(LOGTAG, "Create atrtibute table failed");
			break;
		case ESP_GATTS_READ_EVT:
		case ESP_GATTS_WRITE_EVT:
		case ESP_GATTS_EXEC_WRITE_EVT:
		case ESP_GATTS_MTU_EVT:
		case ESP_GATTS_CONF_EVT:
		case ESP_GATTS_UNREG_EVT:
		case ESP_GATTS_DELETE_EVT:
		case ESP_GATTS_START_EVT:
		case ESP_GATTS_STOP_EVT:
		case ESP_GATTS_OPEN_EVT:
		case ESP_GATTS_CANCEL_OPEN_EVT:
		case ESP_GATTS_CLOSE_EVT:
		case ESP_GATTS_LISTEN_EVT:
		case ESP_GATTS_CONGEST_EVT:
		default:
			break;
	}
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
	esp_ble_gatts_cb_param_t *param)
{
	// If event is register event, store the gatts_if for each profile
	if (event == ESP_GATTS_REG_EVT)
	{
		if (param->reg.status == ESP_GATT_OK)
			dn8_profile_tab[DN8_PROFILE_APP_IDX].gatts_if = gatts_if;
		else
		{
			ESP_LOGI(LOGTAG, "Reg app failed, app_id %04x, status %d\n",
				param->reg.app_id, param->reg.status);
			return;
		}
	}

	do
	{
		int idx;
		for (idx = 0; idx < DN8_PROFILE_NUM; idx++)
		{
			if (gatts_if == ESP_GATT_IF_NONE ||
				gatts_if == dn8_profile_tab[idx].gatts_if)
			{
				if (dn8_profile_tab[idx].gatts_cb)
					dn8_profile_tab[idx].gatts_cb(event, gatts_if, param);
			}
		}
	} while (0);
}


static void init_ble_globals(void)
{
	//This is the 3 byte manufacturer data for advertisements
	dn8_manufacturer[0] = 'D';
	dn8_manufacturer[1] = 'N';
	dn8_manufacturer[2] = '8';
	
	//The security sevice UUID... this is annoying...
	sec_service_uuid[0] = 0xfb;
	sec_service_uuid[1] = 0x34;
	sec_service_uuid[2] = 0x9b;
	sec_service_uuid[3] = 0x5f;
	sec_service_uuid[4] = 0x80;
	sec_service_uuid[5] = 0x00;
	sec_service_uuid[6] = 0x00;
	sec_service_uuid[7] = 0x80;
	sec_service_uuid[8] = 0x00;
	sec_service_uuid[9] = 0x10;
	sec_service_uuid[10] = 0x00;
	sec_service_uuid[11] = 0x00;
	sec_service_uuid[12] = 0x18;
	sec_service_uuid[13] = 0x0d;
	sec_service_uuid[14] = 0x00;
	sec_service_uuid[15] = 0x00;

	// Advertisement configuation
	dn8_adv_config.set_scan_rsp = false;
	dn8_adv_config.include_txpower = true;
	dn8_adv_config.min_interval = 0x100;
	dn8_adv_config.max_interval = 0x100;
	dn8_adv_config.appearance = 0x00;
	dn8_adv_config.manufacturer_len = sizeof(dn8_manufacturer);
	dn8_adv_config.p_manufacturer_data = dn8_manufacturer;
	dn8_adv_config.service_data_len = 0;
	dn8_adv_config.p_service_data = NULL;
	dn8_adv_config.service_uuid_len = sizeof(sec_service_uuid);
	dn8_adv_config.p_service_uuid = sec_service_uuid;
	dn8_adv_config.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

	// Advertisement parameters
	dn8_adv_params.adv_int_min       = 0x100;
	dn8_adv_params.adv_int_max       = 0x100;
	dn8_adv_params.adv_type          = ADV_TYPE_IND;
	dn8_adv_params.own_addr_type     = BLE_ADDR_TYPE_RANDOM;
	dn8_adv_params.channel_map       = ADV_CHNL_ALL;
	dn8_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

	// Scan rsp configuration
	dn8_scan_rsp_config.set_scan_rsp = true;
	dn8_scan_rsp_config.include_name = true;
	dn8_scan_rsp_config.manufacturer_len = sizeof(dn8_manufacturer);
	dn8_scan_rsp_config.p_manufacturer_data = dn8_manufacturer;

	// Setup the application callback handler
	dn8_profile_tab[DN8_PROFILE_APP_IDX].gatts_cb = gatts_profile_event_handler;
	dn8_profile_tab[DN8_PROFILE_APP_IDX].gatts_if = ESP_GATT_IF_NONE;
}

static void init_ble_security(void)
{
	esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND; // Bond w/ device after auth
	esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT; // no output, no input - TODO: ESP_IO_CAP_IO
	uint8_t key_size = 16; // 7-16 bytes in length
	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	uint8_t resp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	uint32_t passkey = 123456; // TODO: Random?
	uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;

	// FIXME: Different passkey? No passkey but use verification?
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req,
		sizeof(esp_ble_auth_req_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(esp_ble_io_cap_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option,
		sizeof(uint8_t));

	// When acting as server, init_key means which types of key client should distribute
	// and resp_key means which keys the server can distribute
	// When acting as client, this is flipped around
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &resp_key, sizeof(uint8_t));
}

static esp_err_t init_ble(void)
{
	esp_err_t ret = 0;

	// Initialize general Bluetooth / Bluedroid
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret)
	{
		ESP_LOGE(LOGTAG, "%s init controller failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}
	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret)
	{
		ESP_LOGE(LOGTAG, "%s enable controller failed %s", __func__, esp_err_to_name(ret));
		return ret;
	}
	ESP_LOGI(LOGTAG, "%s init bluetooth", __func__);
	ret = esp_bluedroid_init();
	if (ret)
	{
		ESP_LOGE(LOGTAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}
	ret = esp_bluedroid_enable();
	if (ret)
	{
		ESP_LOGE(LOGTAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	// Register the gatts register, gap register, and gatts app register callbacks
	ret = esp_ble_gatts_register_callback(gatts_event_handler);
	if (ret)
	{
		ESP_LOGE(LOGTAG, "gatts register error, error code = %x", ret);
		return ret;
	}

	ret = esp_ble_gap_register_callback(gap_event_handler);
	if (ret)
	{
		ESP_LOGE(LOGTAG, "gap register error, error code = %x", ret);
		return ret;
	}

	// TODO: Our APP ID?
	ret = esp_ble_gatts_app_register(DN8_APP_ID);
	if (ret)
	{
		ESP_LOGE(LOGTAG, "gatts app register error, error code = %x", ret);
		return ret;
	}
	else // good to go, setup all the other info
	{
		init_ble_security();
	}
	return ret;
}

bool BluetoothTask::init()
{
	esp_err_t ret = 0;
	ESP_LOGI(LOGTAG, "INIT START");
	pBTTask = this;

	init_ble_globals();
	ret = init_ble();
	if (ret)
		ESP_LOGE(LOGTAG, "BLE Initialization failed\n");
	else
		ESP_LOGI(LOGTAG, "BLE Initialization succeeded\n");
	// TODO:
	// open nvs, get badge name from flash

	// initialize bluetooth device with name, if found, otherwise default to DN8-Dev

	// Create the server
	// Create the UART Service
		// UART UUID
		// Input Characteristic
		// Output Characteristic
		// Descriptor

	// Setup Advertising

	ESP_LOGI(LOGTAG, "INIT COMPLETE");
	return true;
}

BluetoothTask::BluetoothTask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = LOGTAG;
	ESP_LOGI(LOGTAG, "CREATE");
}

BluetoothTask::~BluetoothTask()
{
	// TODO
	ESP_LOGI(LOGTAG, "DESTROY");
}
