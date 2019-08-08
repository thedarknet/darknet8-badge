#include "contact.h"
#include <string.h>
#include <error_type.h>
#include <nvs_memory.h>
#include <esp_log.h>
#include "app.h"

using libesp::ErrorType;
using libesp::NVS;

const char *Contact::LOGTAG = "Contact";

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

const char *Contact::getAgentName() const {
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

void Contact::toStringForSave(char buf[15]) const {
		memset(&buf[0],0,15);
		sprintf(&buf[0],"%02x%02x%02x%02x%02x%02x%02x",
				int32_t(ContactData.ContactID[0]),
				int32_t(ContactData.ContactID[1]),
				int32_t(ContactData.ContactID[2]),
				int32_t(ContactData.ContactID[3]),
				int32_t(ContactData.ContactID[4]),
				int32_t(ContactData.ContactID[5]),
				int32_t(ContactData.ContactID[6])
				);
}


void Contact::toStringPK(char buf[(PUBLIC_KEY_LENGTH*2)+1]) const {
	const uint8_t *PublicKey = &ContactData.PublicKey[0];
	sprintf(&buf[0], 
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x",
		PublicKey[0], PublicKey[1], PublicKey[2], PublicKey[3], 
		PublicKey[4], PublicKey[5], PublicKey[6], PublicKey[7], 
		PublicKey[8], PublicKey[9], PublicKey[10], PublicKey[11], 
		PublicKey[12], PublicKey[13], PublicKey[14], PublicKey[15], 
		PublicKey[16], PublicKey[17], PublicKey[18], PublicKey[19], 
		PublicKey[20], PublicKey[21], PublicKey[22], PublicKey[23], 
		PublicKey[24], PublicKey[25], PublicKey[26], PublicKey[27], 
		PublicKey[28], PublicKey[29], PublicKey[30], PublicKey[31], 
		PublicKey[32], PublicKey[33], PublicKey[34], PublicKey[35], 
		PublicKey[36], PublicKey[37], PublicKey[38], PublicKey[39], 
		PublicKey[40], PublicKey[41], PublicKey[42], PublicKey[43], 
		PublicKey[44], PublicKey[45], PublicKey[46], PublicKey[47], 
		PublicKey[48], PublicKey[49], PublicKey[50], PublicKey[51], 
		PublicKey[52], PublicKey[53], PublicKey[54], PublicKey[55], 
		PublicKey[56], PublicKey[57], PublicKey[58], PublicKey[59], 
		PublicKey[60], PublicKey[61], PublicKey[62], PublicKey[63] 
		);
}

void Contact::toStringSignature(char buf[(SIGNATURE_LENGTH*2)+1]) const {
	const uint8_t *PairSig = &ContactData.PairSig[0];
	sprintf(&buf[0], 
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x"\
			"%02x%02x%02x%02x%02x%02x%02x%02x",
		PairSig[0], PairSig[1], PairSig[2], PairSig[3], 
		PairSig[4], PairSig[5], PairSig[6], PairSig[7], 
		PairSig[8], PairSig[9], PairSig[10], PairSig[11], 
		PairSig[12], PairSig[13], PairSig[14], PairSig[15], 
		PairSig[16], PairSig[17], PairSig[18], PairSig[19], 
		PairSig[20], PairSig[21], PairSig[22], PairSig[23], 
		PairSig[24], PairSig[25], PairSig[26], PairSig[27], 
		PairSig[28], PairSig[29], PairSig[30], PairSig[31], 
		PairSig[32], PairSig[33], PairSig[34], PairSig[35], 
		PairSig[36], PairSig[37], PairSig[38], PairSig[39], 
		PairSig[40], PairSig[41], PairSig[42], PairSig[43], 
		PairSig[44], PairSig[45], PairSig[46], PairSig[47], 
		PairSig[48], PairSig[49], PairSig[50], PairSig[51], 
		PairSig[52], PairSig[53], PairSig[54], PairSig[55], 
		PairSig[56], PairSig[57], PairSig[58], PairSig[59], 
		PairSig[60], PairSig[61], PairSig[62], PairSig[63] 
		);
}

void Contact::dumpContact() {
	char buf[(CONTACT_ID_SIZE*2)+1] = {'\0'};
	toString(buf);
	ESP_LOGI(LOGTAG,"Name: %s", ContactData.AgentName);
	ESP_LOGI(LOGTAG,"Serial ID: %s", &buf[0]);
	char pk[(PUBLIC_KEY_LENGTH*2)+1];
	toStringPK(pk);
	ESP_LOGI(LOGTAG,"Public: %s",pk);
	char sig[(SIGNATURE_LENGTH*2)+1];
	toStringSignature(sig);
	ESP_LOGI(LOGTAG,"Signature %s", sig);
	ESP_LOGI(LOGTAG,"Send To Daemon:\n %s %s", &buf[0], &sig[0]);
}

bool Contact::load(libesp::NVS &nvs, const char *ns) {
	uint32_t len = sizeof(ContactData);
	ErrorType et = nvs.getBlob(ns,&ContactData,len);
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"error saving contact blob: %s", et.toString());
	}
	return et.ok();
}

bool Contact::save(libesp::NVS &nvs) const {
	if(isValidContact()) {
		char buf[(CONTACT_ID_SIZE*2)+1] = {'\0'};
		toStringForSave(buf);
		ErrorType et = nvs.setBlob(&buf[0],&ContactData,sizeof(ContactData));
		if(!et.ok()) {
			ESP_LOGE(LOGTAG,"error saving contact blob: %s", et.toString());
		}
		return et.ok();
	}
	return false;
}

