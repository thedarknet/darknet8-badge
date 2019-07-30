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

