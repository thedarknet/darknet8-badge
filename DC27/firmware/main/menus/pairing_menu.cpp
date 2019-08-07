#include "pairing_menu.h"
#include "menu_state.h"
#include "gui_list_processor.h"
#include "../app.h"
#include "../buttons.h"
#include <app/display_message_state.h>
#include <system.h>
#include <esp_log.h>
#include "../KeyStore.h"

#include <driver/uart.h>
#include "../devkit.h"

using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;

void initialize_pairing_uart()
{
	uart_config_t uart_config {
		.baud_rate           = 115200,
		.data_bits           = UART_DATA_8_BITS,
		.parity              = UART_PARITY_DISABLE,
		.stop_bits           = UART_STOP_BITS_1,
		.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
		.use_ref_tick        = false
	};
	uart_param_config(PAIRING_UART, &uart_config);
	uart_set_pin(PAIRING_UART, PAIRING_TX, PAIRING_RX,
		PAIRING_RTS, PAIRING_CTS);
	uart_driver_install(PAIRING_UART, PAIR_BUFSIZE, PAIR_BUFSIZE,
		0, NULL, 0);
}

PairingMenu::PairingMenu() : DN8BaseMenu() { }

PairingMenu::~PairingMenu() { }

ErrorType PairingMenu::onInit() {
	InternalState = NEGOTIATE;

	uart_flush(PAIRING_UART);

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
	uint32_t buffered_len = 0;

	if (InternalState == NEGOTIATE) {
		if (uart_read_bytes(PAIRING_UART, (uint8_t*)MesgBuf, 1, 20 / portTICK_RATE_MS) > 0)
		{
			if ((MesgBuf[0] == '\x8C') && this->tempAlice) { //AliceConf {
				this->isAlice = false;
				this->isBob = false;
				this->tempAlice = false;
				uart_write_bytes(PAIRING_UART, (const char*)RESET_NEG, 1);
				srand(clock());
				vTaskDelay((rand() % 250) / portTICK_RATE_MS);
			} else if (MesgBuf[0] == '\x8C') { // Alice Confirm 
				this->isBob = true;
				uart_write_bytes(PAIRING_UART, (const char*)BOB_CONFIRM, 1);
				InternalState = BOB_RECEIVE_AIC;
			} else if (MesgBuf[0] == '\x8D') { //BobConfirm
				this->isAlice = true;
				InternalState = ALICE_INIT;
			} else if (MesgBuf[0] == '\x8E') {
				this->isAlice = false;
				this->isBob = false;
				this->tempAlice = false;
			}
		} else if (!this->tempAlice) {
			uart_write_bytes(PAIRING_UART, (const char*)ALICE_CONFIRM, 1);
			this->tempAlice = true;
		}
		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	}
	else if (InternalState == ALICE_INIT) {
		ESP_LOGI(LOGTAG,"ALICE INIT");
		memcpy(&AIC.AlicePublicKey[0], DN8App::get().getContacts().getMyInfo().getPublicKey(), sizeof(AIC.AlicePublicKey));
		memcpy(&AIC.AliceID[0], DN8App::get().getContacts().getMyInfo().getUniqueID(), sizeof(AIC.AliceID));
		strncpy(&AIC.AliceName[0], DN8App::get().getContacts().getSettings().getAgentName(), sizeof(AIC.AliceName));

		uart_write_bytes(PAIRING_UART, (const char*)&AIC, sizeof(AIC));
		DN8App::get().getDisplay().drawString(5, 20, (const char*)"Alice sent AIC", 
							 RGBColor::WHITE);

		this->InternalState = ALICE_RECEIVE_BRTI;
	} else if (InternalState == BOB_RECEIVE_AIC) {
		ESP_LOGI(LOGTAG,"BOB RECEIVE AIC");
		uart_get_buffered_data_len(PAIRING_UART, &buffered_len);
		if (buffered_len == sizeof(AIC)) {
			uart_read_bytes(PAIRING_UART, (uint8_t*)&AIC, sizeof(AIC), 20/portTICK_RATE_MS);

			DN8App::get().getDisplay().drawString(5, 20,
				(const char*)"Bob Received AIC", RGBColor::WHITE);

			if(DN8App::get().getContacts().getMyInfo().sign(AIC.AlicePublicKey,AIC.AliceID,&BRTI.BobSignatureOfAliceData[0])) {
			
				memcpy(&BRTI.BobID[0], DN8App::get().getContacts().getMyInfo().getUniqueID(), sizeof(BRTI.BobID));
				memcpy(&BRTI.BobPublicKey[0],DN8App::get().getContacts().getMyInfo().getPublicKey(), sizeof(BRTI.BobPublicKey));
				strncpy(&BRTI.BobAgentName[0], 
					DN8App::get().getContacts().getSettings().getAgentName()
					,sizeof(BRTI.BobAgentName));

				uart_write_bytes(PAIRING_UART, (const char*)&BRTI, sizeof(BRTI));
				DN8App::get().getDisplay().drawString(5, 30,
					(const char*)"Bob sent BRTI", RGBColor::WHITE);
				InternalState = BOB_RECEIVE_ATBS;
			} else {
				InternalState = PAIRING_FAILED;
			}
		}
		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	}
	else if (InternalState == ALICE_RECEIVE_BRTI) {
		uart_get_buffered_data_len(PAIRING_UART, &buffered_len);
		if (buffered_len == sizeof(BRTI)) {
			ESP_LOGI(LOGTAG,"ALICE_RECEIVE_BRTI");
			uart_read_bytes(PAIRING_UART, (uint8_t *)&BRTI
								 , sizeof(BRTI), 20/portTICK_RATE_MS);

			DN8App::get().getDisplay().drawString(5, 30,
				(const char*)"Alice Received BRTI", RGBColor::WHITE);

			//VERIFY SIGNATURE

			if(DN8App::get().getContacts().getMyInfo().sign(BRTI.BobPublicKey,BRTI.BobID,&ATBS.AliceSignatureOfBobData[0])) {
				uart_write_bytes(PAIRING_UART, (const char*)&ATBS, sizeof(ATBS));
			}

			Contact Bob;
			Bob.setUniqueID(BRTI.BobID);
			Bob.setAgentname(BRTI.BobAgentName);
			Bob.setPublicKey(BRTI.BobPublicKey);
			Bob.setPairingSignature(BRTI.BobSignatureOfAliceData);

			if(DN8App::get().getContacts().addContact(Bob)) {
				ESP_LOGE(LOGTAG,"ALICE SAVED BOB SUCCESSFUL");
				InternalState = PAIRING_SUCCESS;
			} else {
				InternalState = PAIRING_FAILED;
				ESP_LOGE(LOGTAG,"ALICE SAVED BOB FAILED");
			}
		}
		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	} else if (InternalState == BOB_RECEIVE_ATBS) {
		uart_get_buffered_data_len(PAIRING_UART, &buffered_len);
		if (buffered_len == sizeof(ATBS)) {
			ESP_LOGI(LOGTAG,"BOB_RECEIVE_ATBS");
			uart_read_bytes(PAIRING_UART, (uint8_t*) &ATBS, 
								 sizeof(ATBS), 20/portTICK_RATE_MS);

			DN8App::get().getDisplay().drawString(5, 40,
				(const char*)"Bob received ATBS", RGBColor::WHITE);

			InternalState = PAIRING_SUCCESS; // FIXME: verify signatures

			Contact Alice;
			Alice.setUniqueID(AIC.AliceID);
			Alice.setAgentname(AIC.AliceName);
			Alice.setPublicKey(AIC.AlicePublicKey);
			Alice.setPairingSignature(ATBS.AliceSignatureOfBobData);
			
			if(DN8App::get().getContacts().addContact(Alice)) {
				DN8App::get().getDisplay().drawString(5,50,
					(const char *)"Contact Added", RGBColor::BLUE);
				ESP_LOGE(LOGTAG,"BOB SAVED ALICE SUCCESSFUL");
				InternalState = PAIRING_SUCCESS; 
			} else {
				DN8App::get().getDisplay().drawString(5,50,
					(const char *)"Contact failed to save", RGBColor::BLUE);
				ESP_LOGE(LOGTAG,"BOB SAVED ALICE FAILED");
				InternalState = PAIRING_FAILED;
			}
		}
		if (this->timesRunCalledSinceReset > 250)
			InternalState = PAIRING_FAILED;
	} else if (InternalState == PAIRING_SUCCESS) {
		nextState = DN8App::get().getDisplayMessageState(
			DN8App::get().getMenuState(), (const char *)"Pairing Successful", 2000);
	} else if (InternalState == PAIRING_FAILED) {
		nextState = DN8App::get().getDisplayMessageState( DN8App::get().getMenuState(),
			(const char *)"Pairing Failed", 2000);
	}

	this->timesRunCalledSinceReset += 1;
	return ReturnStateContext(nextState);
}

ErrorType PairingMenu::onShutdown() {
	return ErrorType();
}

