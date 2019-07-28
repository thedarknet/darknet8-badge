#ifndef KEY_STORE_H
#define KEY_STORE_H

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
protected:
	struct __attribute__((__packed__)) ContactSaveData {
		uint8_t ContactID[CONTACT_ID_SIZE];
		char AgentName[AGENT_NAME_LENGTH];
		uint8_t PublicKey[PUBLIC_KEY_LENGTH];
		uint8_t PairSig[SIGNATURE_LENGTH];
	};
	ContactSaveData ContactData;
};

class ContactStore {
public:
	static const char *LOGTAG;
	static const uint32_t MAX_CONTACTS = 64;
public:
	class SettingsInfo {
	public:
		SettingsInfo();
		bool init();
		bool setScreenSaverType(uint8_t value);
		uint8_t getScreenSaverType();
		bool setScreenSaverTime(uint8_t value);
		uint8_t getScreenSaverTime();
		bool setSleepTime(uint8_t n);
		uint8_t getSleepTime();
		const char *getAgentName();
		bool isNameSet();
		bool setAgentname(const char name[Contact::AGENT_NAME_LENGTH]);
		void resetToFactory();
	protected:
		struct DataStructure {
			union {
				struct {
					uint32_t ScreenSaverType :4;
					uint32_t SleepTimer :4;
					uint32_t ScreenSaverTime :4;
				} Settings;
				uint32_t Blob;
			};
		};
	private:
		char AgentName[Contact::AGENT_NAME_LENGTH];
		DataStructure Data;
	};

	class MyInfo {
	public:
		MyInfo();
		bool init();
		const uint8_t *getUniqueID();
		bool isUberBadge();
		const uint8_t *getPublicKey();
		bool sign(const uint8_t message[Contact::MESSAGE_DIGEST_SIZE], uint8_t sig[Contact::SIGNATURE_LENGTH]);
	protected:
		uint16_t getFlags();
	private:
		uint8_t UniqueID[Contact::CONTACT_ID_SIZE];
		uint8_t PublicKey[Contact::PUBLIC_KEY_LENGTH];
		uint16_t Flags;
	};
	
public:
	ContactStore();
	MyInfo &getMyInfo();
	SettingsInfo &getSettings();
	bool init();
	bool addContact(const Contact &c);
	uint8_t getNumContacts();
	uint8_t getNumContactsThatCanBeStored();
	void resetToFactory();
protected:
	struct ContactIndex {
		ContactIndex() : NumContacts(0), ConIndex() {}
		uint8_t NumContacts;
		char ConIndex[MAX_CONTACTS][(Contact::CONTACT_ID_SIZE*2)+1];
	};
private:
	SettingsInfo Settings;
	MyInfo MeInfo;
	ContactIndex MyIndex;
};

#endif
