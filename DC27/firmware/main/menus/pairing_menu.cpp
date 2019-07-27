/*
 *
 *      Author: libesp
 */

#include "pairing_menu.h"
#include "menu_state.h"
#include "gui_list_processor.h"
#include "../app.h"
#include "../buttons.h"
#include <system.h>
#include "cryptoauthlib.h"

using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;

PairingMenu::PairingMenu() : DN8BaseMenu(), 
	PairingList("Pairing", 0, 0, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0) {
}

PairingMenu::~PairingMenu() {

}

ErrorType PairingMenu::onInit() {
	InternalState = FETCHING_DATA;

	this->timesRunCalledSinceReset = 0;
	this->msgId = 1;
	CurrentRetryCount = 0;
	memset(&AIC, 0, sizeof(AIC));
	memset(&BRTI, 0, sizeof(BRTI));
	memset(&ATBS, 0, sizeof(ATBS));

	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawString(5, 10, (const char*)"Pairing...", RGBColor::BLUE);

	return ErrorType();
}



BaseMenu::ReturnStateContext PairingMenu::onRun() {

	BaseMenu *nextState = this;

	if (InternalState == NEGOTIATE)
	{
		// TODO: Negotiate Alice and Bob, this may need to be multiple states
			// Alice: ALICE_SEND_AIC
			// Bob: BOB_RECEIVE_AIC
		if (this->timesRunCalledSinceReset > 500)
			InternalState == PAIRING_FAILED;
	}
	else if (InternalState == ALICE_INIT)
	{
		// TODO: AliceInitConvo
		AIC.irmsgid = ALICE_INIT;
		//memcpy(&AIC.AlicePublicKey[0], DN8App::get().getCompressedPublicKey(), // TODO
			//sizeof(AIC.AlicePublicKey));
		AIC.AlicRadioID = DN8App::get().getContacts().getMyInfo().getUniqueId();
		strncpy(&AIC.AliceName[0], DN8APP::get().getContacts.getSettings.getAgentName(),
			sizeof(AIC.AliceName));

		// TODO: Send AIC Message

		this->InternalState = ALICE_RECEIVE_BRTI;
	}
	else if (InternalState == BOB_RECEIVE_AIC)
	{
		if (false) // TODO: Receive AIC
		{
			// TODO: Sign or hash the data
			/* SHA256 code
			uint8_t message_hash[SHA256_HASH_SIZE];
			ShaOBJ messageHashCtx;
			sha256_init(&messageHashCtx);
			sha256_add(&messageHashCtx, (uint8_t*) &aic->AliceRadioID,
				sizeof(aic->AliceRadioID));
			sha256_add(&messageHashCtx, (uint8_t*) &aic->AlicePublicKey,
				sizeof(aic->AlicePublicKey));
			sha256_digest(&messageHashCtx, &message_hash[0]);
			uint8_t signature[SIGNATURE_LENGTH]; // TODO
			uint8_t tmp[32 + 32 + 64];
			SHA256_HashContext ctx = { { &init_SHA256, &update_SHA256, &finish_SHA256, 64, 32,
				&tmp[0] } };
			uECC_sign_deterministic(DN8App::get().getPrivateKey(), // TODO
				message_hash, sizeof(message_hash), &ctx.uECC, signature, THE_CURVE);
			*/

			BRTI.irmsgid = BOB_RECEIVE_AIC;
			//BRTI.BoBRadioID = DN8App::get().getContacts.getMyInfo().getUniqueId();
			//memcpy(&BRTI.BoBPublicKey[0],DN8App::get().getCompressedPublicKey(), // TODO
				//sizeof(BRTI.BoBPublicKey));
			strncpy(&BRTI.BobAgentName[0],
				DN8App::get().getContacts()::getSettings().getAgentName(),
				sizeof(BRTI.BobAgentName));
			memcpy(&BRTI.SignatureOfAliceData[0], &signature[0],
				sizeof(BRTI.SignatureOfAliceData));
			// TODO: Send BRTI Message

			InternalState = BOB_RECEIVE_ATBS;
		}

		if (this->timesRunCalledSinceReset > 500)
			InternalState == PAIRING_FAILED;
	}
	else if (InternalState == ALICE_RECEIVE_BRTI)
	{
		if (false) // TODO: Receive BRTI
		{
			// TODO: Sign or hash data?
			/* SHA256 Code
			uint8_t uncompressedPublicKey[PUBLIC_KEY_LENGTH]; // TODO
			uECC_decompress(&BRTI->BoBPublicKey[0], &uncompressedPublicKey[0], THE_CURVE);
			uint8_t msgHash[SHA256_HASH_SIZE];
			ShaOBJ msgHashCtx;
			sha256_init(&msgHashCtx);
			uint16_t radioID = DN8App::get().getContacts().getMyInfo().getUniqueID();
			sha256_add(&msgHashCtx, (uint8_t*) &radioID, sizeof(uint16_t));
			sha256_add(&msgHashCtx, (uint8_t*) DN8App::get().getCompressedPublicKey(), // TODO
				PUBLIC_KEY_COMPRESSED_LENGTH); // TODO: length definition
			sha256_digest(&msgHashCtx, &msgHash[0]);
			*/

			// TODO: send ATBS
			ATBS.irmsgid = ALICE_RECEIVE_BRTI;
			/* SHA256 Code
			SHA256_HashContext ctx = { { &init_SHA256,&update_SHA256, &finish_SHA256, 64, 32,
				tmp} };
			uECC_sign_deterministic((const unsigned char*)DN8App::get().getPrivateKey(),
				message_hash, sizeof(message_hash), &ctx.uECC, &ATBS.signature[0], THE_CURVE);
			// TODO: actually send ATBS
			*/

			// Add to contacts
			if (!DN8App::get().getContacts().findContactByID(BRTI->BobRadioID, c))
			{
				DN8App::get().getContacts().addContact(BRTI->BobRadioID, &BRTI->BobAgentName[0],
					&BRTI->BoBPublicKey[0], &BRTI->SignatureOfAliceData[0]);
			}

			InternalState = PAIRING_SUCCESS;
		}
		if (this->timesRunCalledSinceReset > 500)
			InternalState == PAIRING_FAILED;
	}
	else if (InternalState == BOB_RECEIVE_ATBS)
	{
		// TODO: finish up

		if (false) // TODO: get ATBS
		{
			/* SHA256 Code
			uint8_t uncompressedPublicKey[PUBLIC_KEY_LENGTH]; // TODO: Public key length
			uECC_decompress(&AIC.AlicePublicKey[0], &uncompressedPublicKey[0], THE_CURVE);
			uint8_t msgHash[SHA256_HASH_SIZE];
			ShaOBJ msgHashCtx;
			sha256_init(&msgHashCtx);
			uint16_t radioID = DN8App::get().getContacts().getMyInfo().getUniqueID(); //TODO
			sha256_add(&msgHashCtx, (uint8_t*) &radioID, sizeof(uint16_t));
			sha256_add(&msgHashCtx, (uint8_t*) DN8App::get().getCompressedPublicKey(), //TODO
				PUBLIC_KEY_COMPRESSED_LENGTH); // TODO
			//verify alice's signature of my public key and unique id
			sha256_digest(&msgHashCtx, &msgHash[0]);
			*/

			/* TODO: Store the data in the contact store
			if (uECC_verify(&uncompressedPublicKey[0], &msgHash[0], sizeof(msgHash),
				&atbs->signature[0], THE_CURVE))
			{
				ContactStore::Contact c;
				if ((DN8App::get().getContacts().findContactByID(AIC.AliceRadioID,c) ||
					DN8App::get().getContacts().addContact(AIC.AliceRadioID,
						&AIC.AliceName[0], &AIC.AlicePublicKey[0], &atbs->signature[0])))
				{
					DN8App::get().getDisplay().drawString(5,40,
						(const char *)"BOB Send Complete", RGBColor::BLUE);
					InternalState = PAIRING_SUCCESS; // Don't initiate the disconnect as Bob
				}
				else
					InternalState = PAIRING_FAILED;
			}
			else
				InternalState = PAIRING_FAILED;
			*/
		}
		if (this->timesRunCalledSinceReset > 500)
			InternalState == PAIRING_FAILED;
	}
	else if (InternalState == PAIRING_SUCCESS)
	{
		// TODO: Display Success
		// TODO: nextState = getMenu()
		if (this->timesRunCalledSinceReset > 500)
			InternalState == PAIRING_FAILED;
	}
	else if (InternalState == PAIRING_FAILED)
	{
		// TODO: Display Error
		// TODO: nextState = getMenu()
		if (this->timesRunCalledSinceReset > 500)
			InternalState == PAIRING_FAILED;
	}

	this->timesRunCalledSinceReset += 1;
	return ReturnStateContext(nextState);
}

ErrorType PairingMenu::onShutdown() {
	return ErrorType();
}

