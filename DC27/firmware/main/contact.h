#ifndef CONTACT_H
#define CONTACT_H

#include <stdint.h>
#include <cryptoauthlib.h>

namespace libesp {
	class NVS;
}

	
class Contact {
public:
	static const uint32_t PUBLIC_KEY_LENGTH = ATCA_PUB_KEY_SIZE;
	static const uint32_t SIGNATURE_LENGTH = ATCA_SIG_SIZE;
	static const uint32_t CONTACT_ID_SIZE = ATCA_SERIAL_NUM_SIZE;
	static const uint32_t MESSAGE_DIGEST_SIZE = ATCA_SHA_DIGEST_SIZE;
	static const uint32_t AGENT_NAME_LENGTH = 12;
	static const uint32_t MAX_CONTACTS = 64;
	static const char *LOGTAG;
public:
	Contact();
	const uint8_t *getUniqueID();
	const char *getAgentName();
	const uint8_t *getPublicKey();
	const uint8_t *getPairingSignature();
	void setUniqueID(uint8_t contactID[CONTACT_ID_SIZE]);
	void setAgentname(const char name[AGENT_NAME_LENGTH]);
	void setPublicKey(uint8_t key[PUBLIC_KEY_LENGTH]);
	void setPairingSignature(const uint8_t sig[SIGNATURE_LENGTH]);
	bool isValidContact() const;
	bool save(libesp::NVS &nvs) const;
	void toString(char buf[CONTACT_ID_SIZE*2+1]) const;
	void toStringForSave(char buf[15]) const;
protected:
	struct __attribute__((__packed__)) ContactSaveData {
		uint8_t ContactID[CONTACT_ID_SIZE];
		char AgentName[AGENT_NAME_LENGTH];
		uint8_t PublicKey[PUBLIC_KEY_LENGTH];
		uint8_t PairSig[SIGNATURE_LENGTH];
	};
	ContactSaveData ContactData;
};


#endif
