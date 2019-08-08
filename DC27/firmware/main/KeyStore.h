#ifndef KEY_STORE_H
#define KEY_STORE_H

#include <stdint.h>
#include "contact.h"
#include "myinfo.h"

namespace libesp {
	class NVS;
}

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
		bool setBLE(bool b);
		bool isBLE();
	protected:
		struct DataStructure {
			union {
				struct {
					uint32_t ScreenSaverType :4;
					uint32_t SleepTimer :4;
					uint32_t ScreenSaverTime :4;
					uint32_t BLE:1;
				} Settings;
				uint32_t Blob;
			};
		};
	private:
		char AgentName[Contact::AGENT_NAME_LENGTH];
		DataStructure Data;
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
	void dumpContacts();
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
