#ifndef KEY_STORE_H
#define KEY_STORE_H

#include <stdint.h>

class ContactStore {
public:
	static const uint32_t PUBLIC_KEY_LENGTH = 1;
	static const uint32_t PUBLIC_KEY_COMPRESSED_LENGTH = 1;
	static const uint32_t SIGNATURE_LENGTH = 1;
	static const uint32_t AGENT_NAME_LENGTH = 12;
	static const uint32_t PUBLIC_KEY_COMPRESSED_STORAGE_LENGTH = 1;
	static const uint32_t MAX_CONTACTS = 64;
public:
	class Contact {
	public:
		friend class ContactStore;
		static const uint8_t SIZE = 0;

		uint16_t getUniqueID();
		const char *getAgentName();
		uint8_t *getCompressedPublicKey();
		void getUnCompressedPublicKey(uint8_t key[PUBLIC_KEY_LENGTH]);
		uint8_t *getPairingSignature();
		void setUniqueID(uint16_t id);
		void setAgentname(const char name[AGENT_NAME_LENGTH]);
		void setCompressedPublicKey(const uint8_t key[PUBLIC_KEY_COMPRESSED_LENGTH]);
		void setPairingSignature(const uint8_t sig[SIGNATURE_LENGTH]);
		Contact(uint32_t startAddress = 0);
		protected:
		uint32_t StartAddress;
	};

	class SettingsInfo {
	public:
		enum {
			CLEAR_ALL = 0x1,
			AVIAN_FLU = 0x2,
			MEASLES	 = 0x4,
			TETANUS = 0x8,
			POLIO 	= 0x10,
			PLAGUE	= 0x20,
			TOXOPLASMOSIS = 0x40,
			CHLAMYDIA	=	0x80,
			HERPES		=	0x100
		};
		static const uint32_t SETTING_MARKER = 0xDCDCDCDC;
		static const uint8_t SIZE = 8 + AGENT_NAME_LENGTH;
		struct DataStructure {
			uint32_t Health :12;
			uint32_t NumContacts :8;
			uint32_t ScreenSaverType :4;
			uint32_t SleepTimer :4;
			uint32_t ScreenSaverTime :4;
		};
	public:
		SettingsInfo(uint8_t sector, uint32_t offSet, uint8_t endSector);
		bool init();
		uint32_t getVersion();
		uint8_t setNumContacts(uint8_t n);
		uint8_t getNumContacts();
		bool getContactAt(uint8_t n, Contact &c);
		bool setScreenSaverType(uint8_t value);
		uint8_t getScreenSaverType();
		bool setScreenSaverTime(uint8_t value);
		uint8_t getScreenSaverTime();
		bool setSleepTime(uint8_t n);
		uint8_t getSleepTime();
		const char *getAgentName();
		bool isNameSet();
		bool setAgentname(const char name[AGENT_NAME_LENGTH]);
		void resetToFactory();
		bool setHealth(uint16_t v);
		uint16_t getHealth();
		bool isInfectedWith(uint16_t v);
		bool cure(uint16_t v);
	protected:
		bool writeSettings(const DataStructure &ds);
		DataStructure getSettings();
		uint32_t getStartAddress();
		uint32_t getEndAddress();
	private:
		uint16_t SettingSector;
		uint32_t OffSet;
		uint8_t EndSettingSector;
		uint32_t CurrentAddress;
		char AgentName[AGENT_NAME_LENGTH];
	};

	class MyInfo {
	public:
		MyInfo(uint32_t startAddress);
		bool init();
		uint16_t getUniqueID();
		uint8_t *getPublicKey();
		uint8_t *getPrivateKey();
		bool isUberBadge();
		uint8_t *getCompressedPublicKey();
	protected:
		uint16_t getFlags();
	private:
		uint32_t StartAddress;
	};

public:
	ContactStore(uint8_t myAddressInfoSector, uint32_t myAddressInfoOffset, uint8_t settingSector, uint32_t settingOffset,
			uint8_t startContactSector, uint8_t endContactSector);
	MyInfo &getMyInfo();
	SettingsInfo &getSettings();
	bool init();
	bool addContact(uint16_t uid, char agentName[AGENT_NAME_LENGTH], uint8_t key[PUBLIC_KEY_LENGTH],
			uint8_t sig[SIGNATURE_LENGTH]);
	uint8_t getNumContactsThatCanBeStored();
	bool getContactAt(uint16_t numContact, Contact &c);
	bool findContactByID(uint16_t uid, Contact &c);
	void resetToFactory();
	private:
	SettingsInfo Settings;
	MyInfo MeInfo;
	uint8_t StartingContactSector;
	uint8_t EndContactSector;
};

#endif
