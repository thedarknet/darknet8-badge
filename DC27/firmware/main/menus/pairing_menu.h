/*
 * pairing_menu.h
 */

#ifndef DARKNET_MENUS_PAIRING_MENU_H_
#define DARKNET_MENUS_PAIRING_MENU_H_

#include "dn8base_menu.h"
#include "../devkit.h"
#include "../contact.h"

void initialize_pairing_uart(void);

class PairingMenu: public DN8BaseMenu {
public:
	struct AliceInitConvo {
		uint8_t AlicePublicKey[Contact::PUBLIC_KEY_LENGTH]; 
		uint8_t AliceID[Contact::CONTACT_ID_SIZE];
      char AliceName[Contact::AGENT_NAME_LENGTH];
	};

	struct BobReplyToInit {
		uint8_t BobPublicKey[Contact::PUBLIC_KEY_LENGTH]; 
		uint8_t BobID[Contact::CONTACT_ID_SIZE];
		char BobAgentName[Contact::AGENT_NAME_LENGTH];
      uint8_t BobSignatureOfAliceData[Contact::SIGNATURE_LENGTH];
    };  

	struct AliceToBobSignature {
		uint8_t AliceSignatureOfBobData[Contact::SIGNATURE_LENGTH];
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
	char MesgBuf[PAIR_BUFSIZE];
	unsigned int MesgLen;

	// Internal State information
	INTERNAL_STATE InternalState;
	uint32_t timesRunCalledSinceReset;

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
	bool isBob   = false;
	bool tempAlice = false;
};


#endif /* DARKNET_MENUS_PAIRING_STATE_H_ */
