#include "myinfo.h"
#include <string.h>
#include <error_type.h>
#include <nvs_memory.h>
#include <esp_log.h>
#include <cryptoauthlib.h>
#include "app.h"

using libesp::ErrorType;
using libesp::NVS;

const char *MyInfo::LOGTAG = "MyInfo";

// MyInfo
//===========================================================
MyInfo::MyInfo():UniqueID(), PublicKey(), Flags(0) {
	memset(&UniqueID[0],0,sizeof(UniqueID));
	memset(&PublicKey[0],0,sizeof(PublicKey));
#ifdef UBER
	Flags = 1;
#endif
}

void MyInfo::logCryptoInfoConfig(const uint8_t *config) {
	ESP_LOGI(LOGTAG,"Serial: %02x%02x%02x%02x%02x%02x%02x%02x%02x",
		int32_t(config[0]), int32_t(config[1]), int32_t(config[2]), int32_t(config[3]), 
		int32_t(config[8]), int32_t(config[9]), int32_t(config[10]), int32_t(config[11]),
	  	int32_t(config[12]));
	ESP_LOGI(LOGTAG,"Rev Number: %02x%02x%02x%02x",
		int32_t(config[4]), int32_t(config[5]), int32_t(config[6]), int32_t(config[7]));
	ESP_LOGI(LOGTAG,"AES Enabled: %d", (int32_t(config[13])&0x1));
	ESP_LOGI(LOGTAG,"I2C Enabled: %d", (int32_t(config[14])&0x1));
	ESP_LOGI(LOGTAG,"I2C Address: %02x", int32_t(config[16]));
	ESP_LOGI(LOGTAG,"CounterMatch: %02x", int32_t(config[18]));
	ESP_LOGI(LOGTAG,"ChipMode I2C ExtraAddress: %d", int32_t(config[19])&0x1);
	ESP_LOGI(LOGTAG,"ChipMode TTL Enabled: %d", int32_t(config[19])&0x2);
	ESP_LOGI(LOGTAG,"ChipMode WatchDog: %s", (int32_t(config[19])&0x4)?"13s":"1.3s");
	for(int i=20,slot=0,k=96;i<52;i+=2,slot++,k+=2) {
		ESP_LOGI(LOGTAG,"	SLOT CONFIG: %d", slot);
		uint16_t v = (*((uint16_t*)&config[i]));
		uint16_t kc = (*((uint16_t*)&config[k]));
		static const int32_t P256ECCKEY = 4;
		static const int32_t AESKEY = 6;
		static const int32_t SHAKEY = 7;
		uint8_t keyType = (config[k]>>2)&0x7;
		if(keyType==P256ECCKEY) {
			ESP_LOGI(LOGTAG,"\t\tP256ECCKEY");
		} else if (keyType==AESKEY) {
			ESP_LOGI(LOGTAG,"\t\tAESKEY");
		} else if (keyType==SHAKEY) {
			ESP_LOGI(LOGTAG,"\t\tSHAKEY");
		} else {
			ESP_LOGI(LOGTAG,"\t\tUNKNOWN");
		}
		if(kc&0x1) {
			ESP_LOGI(LOGTAG,"\t\tPrivate ECC Key in this slot!!!");
			ESP_LOGI(LOGTAG,"\t\tPublic ver of this key can't be generated: %hd",!(kc&0x2));
			ESP_LOGI(LOGTAG,"\t\tPublic ver of this key can be generated: %hd",(kc&0x2)>0?1:0);
			ESP_LOGI(LOGTAG,"\t\tExternal Signatures of messages enabled: %hd", v&0x1);
			ESP_LOGI(LOGTAG,"\t\tinternal Signatures of messages (gendig/genkey)enabled: %hd", (v&0x2)>0?1:0);
			ESP_LOGI(LOGTAG,"\t\tECDH is permitted: %hd", v&0x4);
			ESP_LOGI(LOGTAG,"\t\tECDH master secret output in clear: %hd", v&0x8);
		} else {
			ESP_LOGI(LOGTAG,"\t\tPublic Key in this slot");
			ESP_LOGI(LOGTAG,"\t\tPublic key can be used by verify: %hd",!(kc&0x2));
			ESP_LOGI(LOGTAG,"\t\tPublic key can be used by verify only if public key is validated: %hd",(kc&0x2)>0?1:0);
			ESP_LOGI(LOGTAG,"\t\tRead Key ID: %hd", v&0xF);
		}
		ESP_LOGI(LOGTAG,"\t\tLockable: %hd",(kc&0x20)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tReqRandom: %hd",(kc&0x40)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tReqAuth: %hd",(kc&0x80)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tKey intended for verification: %hd", (v&0x10)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tLimited Use Key: %hd", (v&0x20)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tEncrypted Read: %hd", (v&0x40)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tIsContents Secret: %hd", (v&0x80)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tWrite Key ID: %hd", ((v&0xF00)>>8));
		ESP_LOGI(LOGTAG,"\t\tClear Text writes allowd: %hd",(!(v&0xF000)));
		ESP_LOGI(LOGTAG,"\t\tOnly Allow write if no Key There: %hd",(v&0x1000)>0?1:0);
		ESP_LOGI(LOGTAG,"\t\tNever allow writes: %hd",(v&0x2000)>0?1:0);
	}
	uint64_t Counter0 = (*((uint64_t*)&config[52]));
	ESP_LOGI(LOGTAG,"Counter 0: %llu", Counter0);
	uint64_t Counter1 = (*((uint64_t*)&config[60]));
	ESP_LOGI(LOGTAG,"Counter 1: %llu", Counter1);
	ESP_LOGI(LOGTAG,"Broken UseLock MUST BE 0: %d", config[68]);
	ESP_LOGI(LOGTAG,"Volatile Key Usage (should be 0): %d", config[69]);
	ESP_LOGI(LOGTAG,"Secure Boot functinality: (0) %d", config[70]&0x1);
	ESP_LOGI(LOGTAG,"Lock OTA/DATA zones: (0x55 unlocked) %02x",config[86]);
	ESP_LOGI(LOGTAG,"Lock config zones: (0x55 unlocked) %02x",config[87]);
	uint16_t slotsLocked = (*((uint16_t*)&config[88]));
	ESP_LOGI(LOGTAG,"Slots locked: %02x",int32_t(slotsLocked));
	uint16_t chipOptions = (*((uint16_t*)&config[90]));
	ESP_LOGI(LOGTAG,"Power on Self Test enabled: %hd",chipOptions&0x1);
	ESP_LOGI(LOGTAG,"IOProtection Key enabled: %hd",(chipOptions&0x2)>0?1:0);
	ESP_LOGI(LOGTAG,"KDF AES Enabled: %hd",(chipOptions&0x4)>0?1:0);
	ESP_LOGI(LOGTAG,"Health Test Failed? %hd",(chipOptions&0x8)>0?1:0);

}

uint8_t darknetConfig[] = {
	0x01,0x23,0x8b,0x9f,0x00,0x00,0x60,0x02,0x89,0x46,0xbf,0x0b,0xee,0x01,0x55,0x00, 
	0xc0,0x00,0x00,0x00,0x83,0x20,0x87,0x20,0x8f,0x20,0xc4,0x8f,0x8f,0x8f,0x8f,0x8f,
	0x9f,0x8f,0xaf,0x8f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0xaf,0x8f,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x3f,0xff, 
	0x00,0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
	0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x55,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,
	0x33,0x00,0x33,0x00,0x33,0x00,0x1c,0x00,0x1c,0x00,0x1c,0x00,0x1c,0x00,0x1c,0x00, 
	0x3c,0x00,0x3c,0x00,0x3c,0x00,0x3c,0x00,0x3c,0x00,0x3c,0x00,0x3c,0x00,0x1c,0x00};

bool MyInfo::configConfig() {
	return ATCA_SUCCESS==atcab_write_config_zone(darknetConfig);
}


bool MyInfo::init() {
	ATCAIfaceCfg *gCfg = &cfg_ateccx08a_i2c_default;
	ATCA_STATUS status = ATCA_GEN_FAIL;
	gCfg->iface_type = ATCA_I2C_IFACE,
	gCfg->devtype = ATECC608A,
	gCfg->atcai2c.slave_address = 0xC0; 
	gCfg->atcai2c.bus = 0;
	gCfg->atcai2c.baud = 1000000;
	gCfg->wake_delay = 800;
	gCfg->rx_retries = 20;

	status = atcab_init(gCfg);

	if (status == ATCA_SUCCESS) {
		uint8_t ret[4];
		status = atcab_selftest(SELFTEST_MODE_ALL,0,&ret[0]);
		ESP_LOGI(LOGTAG, "selft test status %d, result: %d",status, int32_t(ret[0]));
		  
		uint8_t configZone[ATCA_ECC_CONFIG_SIZE];
		status = atcab_read_config_zone(&configZone[0]);
		ESP_LOGI(LOGTAG, "Config zone read status %d",status );
		ESP_LOG_BUFFER_HEX(LOGTAG, &configZone[0],sizeof(configZone));
		logCryptoInfoConfig(&configZone[0]);

		bool configZoneLock;
		status = atcab_is_locked(LOCK_ZONE_CONFIG,&configZoneLock);
		if(status == ATCA_SUCCESS) {
			ESP_LOGI(LOGTAG, "isConfigZone Locked? %s ",configZoneLock?DN8App::sYES:DN8App::sNO);
			if(!configZoneLock) {
				if(configConfig()) {
					//lock config
					status = atcab_lock_config_zone();
					ESP_LOGI(LOGTAG, "locking config zone: %d",status );
					status = atcab_is_locked(LOCK_ZONE_CONFIG,&configZoneLock);
					ESP_LOGI(LOGTAG, "verifying lock of config zone: %d: %d",status,configZoneLock );
				}
			} 
			if(configZoneLock) {
				bool dataOTPLock;
				status = atcab_is_locked(LOCK_ZONE_DATA,&dataOTPLock);
				if(status==ATCA_SUCCESS) {
					if(!dataOTPLock) {
						ESP_LOGI(LOGTAG,"Data zone unlocked seeding ...");
						//write OTP
						uint8_t OtaData[ATCA_OTP_SIZE] = 
						{'D','A','R','K','N','E','T',' ','8','\n','C','M','D','C','0','D'
						,'E','\n','G','O','U','R','R','Y',' ', ' ',' ',' ',' ',' ',' ',' '};
						status = atcab_random(&OtaData[32]);
						ESP_LOG_BUFFER_HEX(LOGTAG, &OtaData[0],sizeof(OtaData));
						status = atcab_write_zone(ATCA_ZONE_OTP,0,0,0,&OtaData[0],32);
						ESP_LOGI(LOGTAG,"Write OTP block 0: %d", status);
						status = atcab_write_zone(ATCA_ZONE_OTP,0,1,0,&OtaData[32],32);
						ESP_LOGI(LOGTAG,"Write OTP block 1: %d", status);
						//write key
						status = atcab_genkey(MY_KEY_SLOT,&PublicKey[0]);
						ESP_LOGI(LOGTAG,"GenKey result: %d\nPublic key: ", status);
						ESP_LOG_BUFFER_HEX(LOGTAG, &PublicKey[0],sizeof(PublicKey));
						//LOCKING DATA ZONE
						status = atcab_lock_data_zone();
						ESP_LOGI(LOGTAG,"Locking Data Zone, result: %d", status);
					} else  {
						ESP_LOGI(LOGTAG,"*************************************************");
						ESP_LOGI(LOGTAG,"Data zone locked getting public key of device ...");
						ESP_LOGI(LOGTAG,"*************************************************");
						//just read public key
						status = atcab_get_pubkey(MY_KEY_SLOT,&PublicKey[0]);
						ESP_LOGI(LOGTAG,"Get Public Key Status:  %d\nPublic key: ", status);
						ESP_LOG_BUFFER_HEX(LOGTAG, &PublicKey[0],sizeof(PublicKey));
					}
				} else {
					ESP_LOGE(LOGTAG, "Failed to read data lock: %d",status );
				}
			}
		} else {
			ESP_LOGE(LOGTAG, "Failed to read config lock: %d",status );
		}

#if 1
		uint8_t OtaData[ATCA_OTP_SIZE] = {1};
		status = atcab_read_bytes_zone(ATCA_ZONE_OTP,0,0,&OtaData[0],32);
		if(status == ATCA_SUCCESS) {
			ESP_LOGI(LOGTAG,"OTP data");
			ESP_LOG_BUFFER_HEX(LOGTAG, &OtaData[0],sizeof(OtaData));
		} else {
			ESP_LOGI(LOGTAG,"OTP data read fail: %d", status);
		}
		uint8_t random_number[32];
		status = atcab_random(&random_number[0]);
		ESP_LOGI(LOGTAG,"Random data: status %d", status);
		ESP_LOG_BUFFER_HEX(LOGTAG, &random_number[0],sizeof(random_number));
#endif
		status = atcab_read_serial_number(&UniqueID[0]);
		ESP_LOGI(LOGTAG, "\tSerial: %i", status);
		ESP_LOG_BUFFER_HEX(LOGTAG, &UniqueID[0],sizeof(UniqueID));

		char s[10] = {'\0'};
		status = atcab_version(s);
		ESP_LOGI(LOGTAG, "version: status %d, version %s",status, s );
    } else {
        ESP_LOGE(LOGTAG, "\t- ERROR: %i", status);
    }
	return true;
}

const uint8_t *MyInfo::getUniqueID() {
	return &UniqueID[0];
}

const uint8_t *MyInfo::getPublicKey() {
	return &PublicKey[0];
}

bool MyInfo::isUberBadge() {
	return ((getFlags() & 0x1) != 0);
}

uint16_t MyInfo::getFlags() {
	return Flags;
}


bool DNRandom::generateRandom(int32_t &value) {
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t random_number[32];
	status = atcab_random(&random_number[0]);
	if (status==ATCA_SUCCESS) {
		value = (*((int32_t*)&random_number[0]));
		return true;
	} else {
		value = 0;
		return false;
	}
}

bool DNRandom::generateRandom(uint8_t value[32]) {
	ATCA_STATUS status = ATCA_GEN_FAIL;
	status = atcab_random(&value[0]);
	if (status==ATCA_SUCCESS) {
		return true;
	}
	return false;
}

