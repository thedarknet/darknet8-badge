#ifndef MYINFO_H
#define MYINFO_H

#include <stdint.h>
#include <cryptoauthlib.h>
#include "contact.h"

class DNRandom {
public:
	static bool generateRandom(int32_t &value);
	static bool generateRandom(uint8_t value[32]);
};

class MyInfo {
public:
	static const uint16_t MY_KEY_SLOT = 0;
	static const uint16_t FLAGS_SLOT = 8;
	static const char *LOGTAG;
public:
	MyInfo();
	bool init();
	const uint8_t *getUniqueID();
	bool isUberBadge();
	const uint8_t *getPublicKey();
	bool sign(const uint8_t pk[Contact::PUBLIC_KEY_LENGTH], uint8_t id[Contact::CONTACT_ID_SIZE], uint8_t sig[Contact::SIGNATURE_LENGTH]);
protected:
	uint16_t getFlags();
	void logCryptoInfoConfig(const uint8_t *config);
	bool configConfig();
private:
	uint8_t UniqueID[Contact::CONTACT_ID_SIZE];
	uint8_t PublicKey[Contact::PUBLIC_KEY_LENGTH];
	uint16_t Flags;
};
	
#endif
