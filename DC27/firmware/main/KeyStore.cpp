#include "KeyStore.h"
#include <string.h>


ContactStore::SettingsInfo::SettingsInfo(uint8_t sector, uint32_t offSet, uint8_t endSector) :
		SettingSector(sector), OffSet(offSet), EndSettingSector(endSector), AgentName() {
	CurrentAddress = getStartAddress();
	memset(&AgentName[0], 0, sizeof(AgentName));
}

bool ContactStore::SettingsInfo::init() {
	//couldn't find DS
	CurrentAddress = getEndAddress();
	DataStructure ds;
	ds.Health = 0; //0x1FE; //all the virus
	ds.ScreenSaverTime = 1;
	ds.ScreenSaverType = 0;
	ds.SleepTimer = 3;
	ds.NumContacts = 0;
	return writeSettings(ds);
}

uint32_t ContactStore::SettingsInfo::getStartAddress() {
	return 0;
}

uint32_t ContactStore::SettingsInfo::getEndAddress() {
	return 0;
}

bool ContactStore::SettingsInfo::setAgentname(const char name[AGENT_NAME_LENGTH]) {
	strncpy(&AgentName[0], &name[0], sizeof(AgentName));
	DataStructure ds = getSettings();
	return writeSettings(ds);
}

bool ContactStore::SettingsInfo::setHealth(uint16_t v) {
	DataStructure ds = getSettings();
	if(v==CLEAR_ALL) {
		ds.Health = 0;
	} else {
		ds.Health|=v;
	}
	return writeSettings(ds);
}

uint16_t ContactStore::SettingsInfo::getHealth() {
	DataStructure ds = getSettings();
	return (ds.Health & 0xFFFF);
}

bool ContactStore::SettingsInfo::isInfectedWith(uint16_t v) {
	DataStructure ds = getSettings();
	return ((v&ds.Health)==v);
}

bool ContactStore::SettingsInfo::cure(uint16_t v) {
	DataStructure ds = getSettings();
	ds.Health = (ds.Health&~v);
	return writeSettings(ds);
}

bool ContactStore::SettingsInfo::isNameSet() {
	return (AgentName[0] != '\0' && AgentName[0] != '_');
}

const char *ContactStore::SettingsInfo::getAgentName() {
	return &AgentName[0];
}

uint32_t ContactStore::SettingsInfo::getVersion() {
	return *((uint32_t*) CurrentAddress);
}

uint8_t ContactStore::SettingsInfo::getNumContacts() {
	DataStructure ds = getSettings();
	return ds.NumContacts;
}

ContactStore::SettingsInfo::DataStructure ContactStore::SettingsInfo::getSettings() {
	return *((ContactStore::SettingsInfo::DataStructure*) (CurrentAddress + sizeof(uint32_t)));
}

void ContactStore::SettingsInfo::resetToFactory() {
	{
	}
	init();
}

bool ContactStore::SettingsInfo::writeSettings(const DataStructure &ds) {
	return true;
}

uint8_t ContactStore::SettingsInfo::setNumContacts(uint8_t num) {
	return 0;
}

bool ContactStore::SettingsInfo::setScreenSaverType(uint8_t value) {
	DataStructure ds = getSettings();
	ds.ScreenSaverType = value & 0xF;
	return writeSettings(ds);
}

uint8_t ContactStore::SettingsInfo::getScreenSaverType() {
	DataStructure ds = getSettings();
	return ds.ScreenSaverType;
}

bool ContactStore::SettingsInfo::setScreenSaverTime(uint8_t value) {
	DataStructure ds = getSettings();
	ds.ScreenSaverTime = value & 0xF;
	return writeSettings(ds);
}

uint8_t ContactStore::SettingsInfo::getScreenSaverTime() {
	return getSettings().ScreenSaverTime;
}

bool ContactStore::SettingsInfo::setSleepTime(uint8_t n) {
	DataStructure ds = getSettings();
	ds.SleepTimer = n & 0xF;
	return writeSettings(ds);
}

uint8_t ContactStore::SettingsInfo::getSleepTime() {
	return getSettings().SleepTimer;
}

// MyInfo
//===========================================================
ContactStore::MyInfo::MyInfo(uint32_t startAddress) :
		StartAddress(startAddress) {

}

bool ContactStore::MyInfo::init() {
	return (*(uint16_t*) StartAddress) == 0xdcdc;
}

uint8_t *ContactStore::MyInfo::getPrivateKey() {
	return ((uint8_t*) (StartAddress + sizeof(uint16_t) + sizeof(uint16_t)));
}

uint16_t ContactStore::MyInfo::getUniqueID() {
	return *((uint16_t*) (StartAddress + sizeof(uint16_t)));
}

//TODO make this a member var of MyInfo
uint8_t publicKey[ContactStore::PUBLIC_KEY_LENGTH] = { 0 };
uint8_t *ContactStore::MyInfo::getPublicKey() {
	return 0;
}

uint8_t compressedPublicKey[ContactStore::PUBLIC_KEY_COMPRESSED_STORAGE_LENGTH] = { 0 };
uint8_t *ContactStore::MyInfo::getCompressedPublicKey() {
	return 0;
}

bool ContactStore::MyInfo::isUberBadge() {
	return ((getFlags() & 0x1) != 0);
}

uint16_t ContactStore::MyInfo::getFlags() {
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
ContactStore::Contact::Contact(uint32_t startAddr) :
		StartAddress(startAddr) {
}

uint16_t ContactStore::Contact::getUniqueID() {
	return *((uint16_t*) StartAddress);
}

const char *ContactStore::Contact::getAgentName() {
	return 0;
}

uint8_t *ContactStore::Contact::getCompressedPublicKey() {
	return 0;
}

void ContactStore::Contact::getUnCompressedPublicKey(uint8_t key[PUBLIC_KEY_LENGTH]) {
}

uint8_t *ContactStore::Contact::getPairingSignature() {
	return 0;
}

void ContactStore::Contact::setUniqueID(uint16_t id) {
}

void ContactStore::Contact::setAgentname(const char name[AGENT_NAME_LENGTH]) {
}

void ContactStore::Contact::setCompressedPublicKey(const uint8_t key1[PUBLIC_KEY_COMPRESSED_LENGTH]) {
}

void ContactStore::Contact::setPairingSignature(const uint8_t sig[SIGNATURE_LENGTH]) {
}

//====================================================

ContactStore::MyInfo &ContactStore::getMyInfo() {
	return MeInfo;
}

ContactStore::SettingsInfo &ContactStore::getSettings() {
	return Settings;
}

//ContactStore MyContacts( MyAddressInfoSector, MyAddressInfoOffSet, SettingSector, SettingOffset, StartContactSector, EndContactSector);
//=============================================
ContactStore::ContactStore(uint8_t myAddressInfoSector, uint32_t myAddressInfoOffset, uint8_t settingSector,
	uint32_t settingOffset, uint8_t startContactSector, uint8_t endContactSector) : Settings(settingSector,settingOffset,settingSector+1), MeInfo(0), 
		StartingContactSector(startContactSector), EndContactSector(endContactSector) {

}

void ContactStore::resetToFactory() {
	getSettings().resetToFactory();
}

bool ContactStore::init() {
	if (getMyInfo().init() && Settings.init()) {
		return true;
	}
	return false;
}

bool ContactStore::getContactAt(uint16_t numContact, Contact &c) {
	return false;
}

bool ContactStore::findContactByID(uint16_t uid, Contact &c) {
	return false;
}

bool ContactStore::addContact(uint16_t uid, char agentName[AGENT_NAME_LENGTH],
		uint8_t key[PUBLIC_KEY_COMPRESSED_LENGTH], uint8_t sig[SIGNATURE_LENGTH]) {

	return false;
}

uint8_t ContactStore::getNumContactsThatCanBeStored() {
	return MAX_CONTACTS;
}

