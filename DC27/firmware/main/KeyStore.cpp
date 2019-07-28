#include "KeyStore.h"
#include <string.h>
#include <error_type.h>
#include <nvs_memory.h>
#include <esp_log.h>
#include <cryptoauthlib.h>

using libesp::ErrorType;
using libesp::NVS;

static NVS NVSContact("nvs","contact", false);

/////////////////////////////////////////////////////////////////////////////////
// Contact
Contact::Contact() : ContactData() {
	memset(&ContactData,0,sizeof(ContactData));
}

bool Contact::isValidContact() const {
	int ret = 0;
	for(int i=0;i<Contact::CONTACT_ID_SIZE && ret==0;++i) {
		if(ContactData.ContactID[i]!=0) ret|=1;
	}
	if(ret && ContactData.AgentName[0]!='\0') {
		ret = 0;
		for(int i=0;i<Contact::PUBLIC_KEY_LENGTH && ret==0;++i) {
			if(ContactData.PublicKey[i]!=0) ret|=1;
		}
		if(ret!=0) {
			ret = 0;
			for(int i=0;i<Contact::SIGNATURE_LENGTH && ret==0;++i) {
				if(ContactData.PairSig[i]!=0) ret|=1;
			}
		}
	}
	return ret==1;
}

const  uint8_t *Contact::getUniqueID() {
	return &ContactData.ContactID[0];
}

const char *Contact::getAgentName() {
	return &ContactData.AgentName[0];
}

const uint8_t *Contact::getPublicKey() {
	return &ContactData.PublicKey[0];
}

const uint8_t *Contact::getPairingSignature() {
	return &ContactData.PairSig[0];
}

void Contact::setUniqueID(uint8_t contactID[Contact::CONTACT_ID_SIZE]) {
	memcpy(&ContactData.ContactID[0],&contactID[0],Contact::CONTACT_ID_SIZE);
}

void Contact::setAgentname(const char name[Contact::AGENT_NAME_LENGTH]) {
	memcpy(&ContactData.AgentName[0],&name[0],Contact::AGENT_NAME_LENGTH);
}

void Contact::setPublicKey(uint8_t key[Contact::PUBLIC_KEY_LENGTH]) {
	memcpy(&ContactData.PublicKey[0],&key[0],Contact::PUBLIC_KEY_LENGTH);
}

void Contact::setPairingSignature(const uint8_t sig[Contact::SIGNATURE_LENGTH]) {
	memcpy(&ContactData.PairSig[0],&sig[0],Contact::SIGNATURE_LENGTH);
}

void Contact::toString(char buf[CONTACT_ID_SIZE*2+1]) const {
		sprintf(&buf[0],"%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				int32_t(ContactData.ContactID[0]),
				int32_t(ContactData.ContactID[1]),
				int32_t(ContactData.ContactID[2]),
				int32_t(ContactData.ContactID[3]),
				int32_t(ContactData.ContactID[4]),
				int32_t(ContactData.ContactID[5]),
				int32_t(ContactData.ContactID[6]),
				int32_t(ContactData.ContactID[7]),
				int32_t(ContactData.ContactID[8]));
}

bool Contact::save(libesp::NVS &nvs) const {
	if(isValidContact()) {
		char buf[(CONTACT_ID_SIZE*2)+1] = {'\0'};
		toString(buf);
		ErrorType et = nvs.setBlob(&buf[0],&ContactData,sizeof(ContactData));
		return et.ok();
	}
	return false;
}

////////////////////////////////////////////////
static NVS NVSMe("nvs","me", false);
const char *ContactStore::LOGTAG = "ContactStore";

//////////////////////////////////////////////////////////////////
// SettingInfo
ContactStore::SettingsInfo::SettingsInfo() : AgentName(), Data() {
	Data.Settings.ScreenSaverTime = 1;
	Data.Settings.ScreenSaverType = 0;
	Data.Settings.SleepTimer = 3;
	memset(&AgentName[0], 0, sizeof(AgentName));
}

bool ContactStore::SettingsInfo::init() {
	ErrorType et;
	if(NVSMe.getValue("mysetting",Data.Blob)==ESP_ERR_NVS_NOT_FOUND) {
		Data.Settings.ScreenSaverTime = 1;
		Data.Settings.ScreenSaverType = 0;
		Data.Settings.SleepTimer = 3;
		et = NVSMe.setValue("mysetting",Data.Blob);
	}
	if(et.ok()) {
		uint32_t length = sizeof(AgentName);
		NVSMe.getValue("myname",&AgentName[0],length);
		if(et.ok()) {
			if(!(et = NVSMe.getValue("mysetting",Data.Blob)).ok()) {
				ESP_LOGE(LOGTAG,"%s",et.toString());
			}
		} else {
			ESP_LOGE(LOGTAG,"%s",et.toString());
		}
	} else {
		ESP_LOGE(LOGTAG,"%s",et.toString());
	}
	return et.ok();
}

bool ContactStore::SettingsInfo::setAgentname(const char name[Contact::AGENT_NAME_LENGTH]) {
	strncpy(&AgentName[0], &name[0], sizeof(AgentName));
	if(NVSMe.setValue("myname",&AgentName[0]).ok()) {
		return NVSMe.commit().ok();
	}
	return false;
}

bool ContactStore::SettingsInfo::isNameSet() {
	return (AgentName[0] != '\0' && AgentName[0] != '_');
}

const char *ContactStore::SettingsInfo::getAgentName() {
	return &AgentName[0];
}

void ContactStore::SettingsInfo::resetToFactory() {
	NVSMe.eraseKey("myname");
	NVSMe.eraseKey("mysetting");
	NVSMe.commit();
	init();
}

bool ContactStore::SettingsInfo::setScreenSaverType(uint8_t value) {
	Data.Settings.ScreenSaverType = value & 0xF;
	return NVSMe.setValue("mysetting",Data.Blob).ok();
}

uint8_t ContactStore::SettingsInfo::getScreenSaverType() {
	return Data.Settings.ScreenSaverType;
}

bool ContactStore::SettingsInfo::setScreenSaverTime(uint8_t value) {
	Data.Settings.ScreenSaverTime = value & 0xF;
	return NVSMe.setValue("mysetting",Data.Blob).ok();
}

uint8_t ContactStore::SettingsInfo::getScreenSaverTime() {
	return Data.Settings.ScreenSaverTime;
}

bool ContactStore::SettingsInfo::setSleepTime(uint8_t n) {
	Data.Settings.SleepTimer = n & 0xF;
	return NVSMe.setValue("mysetting",Data.Blob).ok();
}

uint8_t ContactStore::SettingsInfo::getSleepTime() {
	return Data.Settings.SleepTimer;
}

// MyInfo
//===========================================================
ContactStore::MyInfo::MyInfo():UniqueID(), PublicKey(), Flags(0) {
	memset(&UniqueID[0],0,sizeof(UniqueID));
	memset(&PublicKey[0],0,sizeof(PublicKey));
}

bool ContactStore::MyInfo::init() {
	//init crytpo
	//load serial
	//load public key
	//load flags
	ATCAIfaceCfg *gCfg = &cfg_ateccx08a_i2c_default;
	ATCA_STATUS status = ATCA_GEN_FAIL;
	uint8_t random_number[32];
	uint8_t serial_number[9];

	gCfg->iface_type = ATCA_I2C_IFACE,
	gCfg->devtype = ATECC608A,
	gCfg->atcai2c.slave_address = 0xC0;  // Detected correctly!!
	gCfg->atcai2c.bus = 0;
	gCfg->atcai2c.baud = 400000;
	gCfg->wake_delay = 800;
	gCfg->rx_retries = 20;

	status = atcab_init(gCfg);

	if (status == ATCA_SUCCESS) {
		uint8_t ret[4];
		status = atcab_selftest(SELFTEST_MODE_ALL,0,&ret[0]);
		ESP_LOGI(LOGTAG, "selft test status %d, result: %d",status, int32_t(ret[0]));
		  
		status = atcab_info(&ret[0]);
		ESP_LOGI(LOGTAG, "info status %d",status );
		ESP_LOG_BUFFER_HEX(LOGTAG, &ret[0],sizeof(ret));

		uint32_t counter = 0;
		status = atcab_counter_read(1,&counter);
		ESP_LOGI(LOGTAG, "counter status %d, counter %d",status, counter );
		status = atcab_counter_increment(1,&counter);
		ESP_LOGI(LOGTAG, "counter status %d, counter %d",status, counter );
		status = atcab_counter_increment(1,&counter);
		ESP_LOGI(LOGTAG, "counter status %d, counter %d",status, counter );

		status = atcab_random(random_number);
		ESP_LOGI(LOGTAG, "\tRandom: %i", status);
		ESP_LOG_BUFFER_HEX(LOGTAG, &random_number[0],sizeof(random_number));

		status = atcab_read_serial_number(serial_number);
		ESP_LOGI(LOGTAG, "\tSerial: %i", status);
		ESP_LOG_BUFFER_HEX(LOGTAG, &serial_number[0],sizeof(serial_number));

		char s[10] = {'\0'};
		status = atcab_version(s);
		ESP_LOGI(LOGTAG, "version: status %d, version %s",status, s );
		//status = atcab_release();
		//ESP_LOGI(LOGTAG, "\tRelease: %i", status);
    } else {
        ESP_LOGE(LOGTAG, "\t- ERROR: %i", status);
    }
	return true;
}

const uint8_t *ContactStore::MyInfo::getUniqueID() {
	return &UniqueID[0];
}

const uint8_t *ContactStore::MyInfo::getPublicKey() {
	return &PublicKey[0];
}

bool ContactStore::MyInfo::isUberBadge() {
	return ((getFlags() & 0x1) != 0);
}

uint16_t ContactStore::MyInfo::getFlags() {
	return Flags;
}


//====================================================
ContactStore::MyInfo &ContactStore::getMyInfo() {
	return MeInfo;
}

ContactStore::SettingsInfo &ContactStore::getSettings() {
	return Settings;
}

//=============================================
ContactStore::ContactStore() : Settings(), MeInfo(), MyIndex() {
	memset(&MyIndex,0,sizeof(MyIndex));
}

void ContactStore::resetToFactory() {
	getSettings().resetToFactory();
}

bool ContactStore::init() {
	ErrorType et = NVSContact.init();
	if(et.ok()) {
		NVSContact.logInfo();
		if(!(et = NVSMe.init()).ok()) {
			ESP_LOGE(LOGTAG,"MyInfo init: %s", et.toString());
		}
		NVSMe.logInfo();
	} else {
		ESP_LOGE(LOGTAG,"MyInfo init: %s", et.toString());
	}
	if (et.ok() && getMyInfo().init() && Settings.init()) {
		uint32_t len = sizeof(MyIndex);
		et = NVSContact.getBlob("MyIndex",&MyIndex,len);
		if(et==ESP_ERR_NVS_NOT_FOUND || et.ok()) {
			return true;
		} 
	}
	return false;
}

bool ContactStore::addContact(const Contact &c) {
	if(c.save(NVSContact)) {
		char buf[Contact::CONTACT_ID_SIZE*2+1];
		c.toString(buf);
		strcpy(MyIndex.ConIndex[MyIndex.NumContacts], &buf[0]);
		MyIndex.NumContacts++;
		ErrorType et = NVSContact.setBlob("MyIndex",&MyIndex,sizeof(MyIndex));
		if(et.ok()) {
			return true;
		} else {
			ESP_LOGE(LOGTAG,"add contact fail: %s", et.toString());
		}
	}
	return false;
}

uint8_t ContactStore::getNumContactsThatCanBeStored() {
	return MAX_CONTACTS;
}

uint8_t ContactStore::getNumContacts() {
	return MyIndex.NumContacts;
}

