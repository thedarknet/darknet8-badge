/*
 * pairing_menu.h
 */

#ifndef DARKNET_MENUS_PAIRING_MENU_H_
#define DARKNET_MENUS_PAIRING_MENU_H_

#include "dn8base_menu.h"

class ContactStore;

class PairingMenu: public DN8BaseMenu {
public:
	struct AliceInitConvo {
        uint8_t irmsgid;
        uint8_t AlicePublicKey[ContactStore::PUBLIC_KEY_COMPRESSED_LENGTH];
        uint16_t AliceRadioID;
        char AliceName[ContactStore::AGENT_NAME_LENGTH];
	};

    struct BobReplyToInit {
        uint8_t irmsgid;
        uint8_t BoBPublicKey[ContactStore::PUBLIC_KEY_COMPRESSED_LENGTH];
        uint16_t BoBRadioID;
        char BobAgentName[ContactStore::AGENT_NAME_LENGTH];
        uint8_t SignatureOfAliceData[ContactStore::SIGNATURE_LENGTH];
    };  

    struct AliceToBobSignature {
        uint8_t irmsgid;
        uint8_t signature[48];
    };

	PairingMenu();
	virtual ~PairingMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();

	enum INTERNAL_STATE { NONE, NEGOTIATE, 
		ALICE_INIT, BOB_RECEIVE_AIC, ALICE_RECEIVE_BRTI, BOB_RECEIVE_ATBS,
		PAIRING_SUCCESS, PAIRING_FAILED };

private:
	char MesgBuf[200];
	unsigned int MesgLen;

	// Internal State information
	INTERNAL_STATE InternalState;
	uint32_t ESPRequestID;
	uin32_t timesRunCalledSinceReset;

	// Pairing State information
	uint16_t TimeoutMS;
	uint8_t RetryCount;
	uint8_t CurrentRetryCount;
	uint32_t TimeInState;
	AliceInitConvo AIC;
	BobReplyToInit BRTI;
	AliceToBobSignature ATBS;
	unsigned char msgId = 0;
	bool isAlice = false;
};


#endif /* DARKNET_MENUS_PAIRING_STATE_H_ */
