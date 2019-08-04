#include "pairing_menu.h"
#include "menu_state.h"
#include "gui_list_processor.h"
#include "../app.h"
#include "../buttons.h"
#include <app/display_message_state.h>
#include <system.h>
//#include "cryptoauthlib.h"
#include <esp_log.h>

#include <driver/uart.h>
#include "../devkit.h"

using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;

PairingMenu::PairingMenu() : DN8BaseMenu() { }

PairingMenu::~PairingMenu() { }

ErrorType PairingMenu::onInit() {
	InternalState = NEGOTIATE;

	this->isAlice = false;
	this->isBob = false;
	this->tempAlice = false;

	this->timesRunCalledSinceReset = 0;
	this->msgId = 1;
	CurrentRetryCount = 0;
	memset(&AIC, 0, sizeof(AIC));
	memset(&BRTI, 0, sizeof(BRTI));
	memset(&ATBS, 0, sizeof(ATBS));
	memset(MesgBuf, 0, PAIR_BUFSIZE);
	this->buffered_len = 0;

	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getDisplay().drawString(5, 10,
		(const char*)"Negotiating...", RGBColor::WHITE);

	return ErrorType();
}

#define ALICE_CONFIRM "\x8C"
#define BOB_CONFIRM   "\x8D"
#define RESET_NEG     "\x8E"

BaseMenu::ReturnStateContext PairingMenu::onRun() {

	BaseMenu *nextState = this;

	if (InternalState == NEGOTIATE)
	{
		if (uart_read_bytes(PAIRING_UART, (uint8_t*)MesgBuf, 1,
			20 / portTICK_RATE_MS) > 0)
		{
			if ((MesgBuf[0] == '\x8C') && this->tempAlice) //AliceConf
			{
				this->isAlice = false;
				this->isBob = false;
				this->tempAlice = false;
				uart_write_bytes(PAIRING_UART, (const char*)RESET_NEG, 1);
				srand(clock());
				vTaskDelay((rand() % 250) / portTICK_RATE_MS);
			}
			else if (MesgBuf[0] == '\x8C') // Alice Confirm
			{
				this->isBob = true;
				uart_write_bytes(PAIRING_UART, (const char*)BOB_CONFIRM, 1);
				InternalState = BOB_RECEIVE_AIC;
			}
			else if (MesgBuf[0] == '\x8D') // Bob Confirm
			{
				this->isAlice = true;
				InternalState = ALICE_INIT;
			}
			else if (MesgBuf[0] == '\x8E') // Reset Negotiation
			{
				this->isAlice = false;
				this->isBob = false;
				this->tempAlice = false;
			}
		}
		else if (!this->tempAlice)
		{
			uart_write_bytes(PAIRING_UART, (const char*)ALICE_CONFIRM, 1);
			this->tempAlice = true;
		}
		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	}
	else if (InternalState == ALICE_INIT)
	{
		AIC.irmsgid = ALICE_INIT;

		// TODO: Fill in AIC
		//memcpy(&AIC.AlicePublicKey[0], DN8App::get().getCompressedPublicKey(), // TODO
			//sizeof(AIC.AlicePublicKey));
		//AIC.AliceRadioID = DN8App::get().getContacts().getMyInfo().getUniqueId();
		//strncpy(&AIC.AliceName[0], DN8APP::get().getContacts.getSettings.getAgentName(),
			//sizeof(AIC.AliceName));

		uart_write_bytes(PAIRING_UART, (const char*)&AIC, sizeof(AIC));
		DN8App::get().getDisplay().drawString(5, 20,
			(const char*)"Alice sent AIC", RGBColor::WHITE);

		this->InternalState = ALICE_RECEIVE_BRTI;
	}
	else if (InternalState == BOB_RECEIVE_AIC)
	{
		uart_get_buffered_data_len(PAIRING_UART, &buffered_len);
		if (buffered_len == sizeof(AIC))
		{
			uint8_t signature[128]; // FIXME: magic number
			this->buffered_len = 0; // reset for next step
			uart_read_bytes(PAIRING_UART, (uint8_t*)MesgBuf, sizeof(AIC),
				20/portTICK_RATE_MS);
			DN8App::get().getDisplay().drawString(5, 20,
				(const char*)"Bob Received AIC", RGBColor::WHITE);

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
			//strncpy(&BRTI.BobAgentName[0],
			//	DN8App::get().getContacts().getSettings().getAgentName(), // FIXME
			//	sizeof(BRTI.BobAgentName));
			memcpy(&BRTI.SignatureOfAliceData[0], &signature[0],
				sizeof(BRTI.SignatureOfAliceData));

			uart_write_bytes(PAIRING_UART, (const char*)&BRTI, sizeof(BRTI));
			DN8App::get().getDisplay().drawString(5, 30,
				(const char*)"Bob sent BRTI", RGBColor::WHITE);

			InternalState = BOB_RECEIVE_ATBS;
		}

		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	}
	else if (InternalState == ALICE_RECEIVE_BRTI)
	{
		uart_get_buffered_data_len(PAIRING_UART, &buffered_len);
		if (buffered_len == sizeof(BRTI))
		{
			uart_read_bytes(PAIRING_UART, (uint8_t*)MesgBuf, sizeof(AIC),
				20/portTICK_RATE_MS);
			DN8App::get().getDisplay().drawString(5, 30,
				(const char*)"Alice Received BRTI", RGBColor::WHITE);
			this->buffered_len = 0; // clear for the next step;
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

			uart_write_bytes(PAIRING_UART, (const char*)&ATBS, sizeof(ATBS));

			// TODO Add to contacts
			/*
			if (!DN8App::get().getContacts().findContactByID(BRTI->BobRadioID, c))
			{
				DN8App::get().getContacts().addContact(BRTI->BobRadioID, &BRTI->BobAgentName[0],
					&BRTI->BoBPublicKey[0], &BRTI->SignatureOfAliceData[0]);
			}
			*/

			InternalState = PAIRING_SUCCESS;
		}
		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	}
	else if (InternalState == BOB_RECEIVE_ATBS)
	{
		uart_get_buffered_data_len(PAIRING_UART, &buffered_len);
		if (buffered_len == sizeof(ATBS))
		{
			uart_read_bytes(PAIRING_UART, (uint8_t*)MesgBuf, sizeof(AIC),
				20/portTICK_RATE_MS);
			DN8App::get().getDisplay().drawString(5, 40,
				(const char*)"Bob received ATBS", RGBColor::WHITE);
			this->buffered_len = 0; // clear for the next step;

			InternalState = PAIRING_SUCCESS; // FIXME: remove

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
		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	}
	else if (InternalState == PAIRING_SUCCESS)
	{
		nextState = DN8App::get().getDisplayMessageState(
			DN8App::get().getMenuState(),
			(const char *)"Pairing Successful", 2000);
	}
	else if (InternalState == PAIRING_FAILED)
	{
		nextState = DN8App::get().getDisplayMessageState(
			DN8App::get().getMenuState(),
			(const char *)"Pairing Failed", 2000);
	}

	this->timesRunCalledSinceReset += 1;
	return ReturnStateContext(nextState);
}

ErrorType PairingMenu::onShutdown() {
	return ErrorType();
}

