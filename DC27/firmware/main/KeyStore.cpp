#include "KeyStore.h"
#include <string.h>
#include <error_type.h>
#include <nvs_memory.h>
#include <esp_log.h>
#include "app.h"

using libesp::ErrorType;
using libesp::NVS;

static NVS NVSContact("nvs","contact", false);
static NVS NVSMe("nvs","me", false);
const char *ContactStore::LOGTAG = "ContactStore";

//////////////////////////////////////////////////////////////////
// SettingInfo
ContactStore::SettingsInfo::SettingsInfo() : AgentName(), Data() {
	Data.Settings.ScreenSaverTime = 5;
	Data.Settings.ScreenSaverType = 0;
	Data.Settings.SleepTimer = 3;
	Data.Settings.BLE = 0;
	memset(&AgentName[0], 0, sizeof(AgentName));
}

bool ContactStore::SettingsInfo::init() {
	ErrorType et;
	if(NVSMe.getValue("mysetting",Data.Blob)==ESP_ERR_NVS_NOT_FOUND) {
		Data.Settings.ScreenSaverTime = 1;
		Data.Settings.ScreenSaverType = 0;
		Data.Settings.SleepTimer = 3;
		Data.Settings.BLE = 0;
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

bool ContactStore::SettingsInfo::setBLE(bool b) {
	Data.Settings.BLE = b;
	return NVSMe.setValue("mysetting",Data.Blob).ok();
}

bool ContactStore::SettingsInfo::isBLE() {
	return Data.Settings.BLE==1;
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

//====================================================
MyInfo &ContactStore::getMyInfo() {
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
	NVSContact.wipe();
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

void ContactStore::dumpContacts() {
	ESP_LOGI(LOGTAG,"**********************************************");
	ESP_LOGI(LOGTAG,"****************Contacts**********************");
	ESP_LOGI(LOGTAG,"**********************************************");
	for(int i=0;i<MyIndex.NumContacts;++i) {
		ESP_LOGI(LOGTAG,"**********************************************");
		Contact c;
		if(c.load(NVSContact,MyIndex.ConIndex[i])) {
			c.dumpContact();
		} else {
			ESP_LOGE(LOGTAG,"Failed to load contact");
		}
		ESP_LOGI(LOGTAG,"**********************************************");
	}
	ESP_LOGI(LOGTAG,"**********************************************");
	ESP_LOGI(LOGTAG,"****************Contacts**********************");
	ESP_LOGI(LOGTAG,"**********************************************");
}

bool ContactStore::addContact(const Contact &c) {
	if(c.save(NVSContact)) {
		char buf[15];
		c.toStringForSave(buf);
		//verify not already in list
		for(int i=0;i<MyIndex.NumContacts;++i) {
			if(0==strcmp(MyIndex.ConIndex[MyIndex.NumContacts],&buf[0])) {
				ESP_LOGI(LOGTAG,"already paired with %s", c.getAgentName());
				return true;
			}
		}
		strcpy(MyIndex.ConIndex[MyIndex.NumContacts], &buf[0]);
		MyIndex.NumContacts++;
		ErrorType et = NVSContact.setBlob("MyIndex",&MyIndex,sizeof(MyIndex));
		if(et.ok()) {
			return true;
		} else {
			ESP_LOGE(LOGTAG,"add contact fail: %s", et.toString());
		}
	}
	ESP_LOGE(LOGTAG,"FAILED TO SAVE CONTACT:");
	return false;
}

uint8_t ContactStore::getNumContactsThatCanBeStored() {
	return MAX_CONTACTS;
}

uint8_t ContactStore::getNumContacts() {
	return MyIndex.NumContacts;
}

