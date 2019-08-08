#include "menu_state.h"
#include "../app.h"
#include "gui_list_processor.h"
#include "../buttons.h"
#include "calibration_menu.h"
#include "test_menu.h"
#include "scan.h"
#include "../KeyStore.h"
#include "setting_state.h"
#include <app/display_message_state.h>
#include "game_of_life.h"
#include "badge_info_menu.h"
#include "pairing_menu.h"
#include "communications_settings.h"
#include "menu3d.h"
#include "top_board.h"
#include <esp_log.h>
#include "drawing.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::XPT2046;
using libesp::Point2Ds;
using libesp::TouchNotification;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[MenuState::QUEUE_SIZE*MenuState::MSG_SIZE] = {0};
static const char *LOGTAG = "MenuState";

MenuState::MenuState() :
	DN8BaseMenu(), MenuList("Main Menu", Items, 0, 0, 
	DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel()
	, 0, ItemCount) {
	
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

MenuState::~MenuState() {

}


ErrorType MenuState::onInit() {
	Items[0].id = 0;
	if (DN8App::get().getContacts().getSettings().isNameSet()) {
		Items[0].text = (const char *) "Settings";
	} else {
		Items[0].text = (const char *) "Settings *";
	}
	Items[1].id = 1;
	Items[1].text = (const char *) "Badge Pair";
	Items[2].id = 2;
	Items[2].text = (const char *) "3D";
	Items[3].id = 3;
	Items[3].text = (const char *) "Screen Saver";
	Items[4].id = 4;
	Items[4].text = (const char *) "Draw";
	Items[5].id = 5;
	Items[5].text = (const char *) "Badge Info";
	Items[6].id = 6;
	Items[6].text = (const char *) "Communications Settings";
	//Items[7].id = 7;
	//Items[7].text = (const char *) "Scan for NPCs ";
	Items[7].id = 7;
	Items[7].text = (const char *) "Test Badge";
	Items[8].id = 8;
	Items[8].text = (const char *) "Calibrate Touch";
	Items[9].id = 9;
	Items[9].text = (const char *) "Connected Devices";
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawList(&this->MenuList);
	//empty queue
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	DN8App::get().getTouch().addObserver(InternalQueueHandler);

	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext MenuState::onRun() {
	BaseMenu *nextState = this;
	TouchNotification *pe = nullptr;
	bool penUp = false;
	bool hdrHit = false;
	pe = processTouch(InternalQueueHandler, MenuList, ItemCount, penUp, hdrHit);
	if (pe || !GUIListProcessor::process(&MenuList,ItemCount)) {
		if (penUp || selectAction()) {
			switch (MenuList.getSelectedItemID()) {
				case 0:
					nextState = DN8App::get().getSettingsMenu();
					break;
				case 1:
					if (DN8App::get().getContacts().getSettings().getAgentName()[0]!='\0') {
						nextState = DN8App::get().getPairingMenu();
					} else {
						nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(),
								(const char *) "You must set your agent name first", 3000);
					}
					break;
				case 2:
					nextState = DN8App::get().get3DMenu();
					break;
				case 3:
					nextState = DN8App::get().getGameOfLifeMenu();
					break;
				case 4:
					nextState = DN8App::get().getDrawingMenu();
					break;
				case 5:
					nextState = DN8App::get().getBadgeInfoMenu();
					break;
				case 6:
					nextState = DN8App::get().getCommunicationSettingState();
					break;
					/*
				case 7:
					if(DN8App::get().getWifiTask()) {
						DN8App::get().getWifiScanMenu()->setNPCOnly(true);
						nextState = DN8App::get().getWifiScanMenu();
					} else {
						nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *) "Enable WiFi first in settings", 3000);
					}
					break;
					*/
				case 7:
					nextState = DN8App::get().getTestMenu();
					break;
				case 8:
					nextState = DN8App::get().getCalibrationMenu();
					break;
				case 9:
					nextState = DN8App::get().getTopBoardMenu();
					break;
			}
		}
	}

	if (DN8App::get().getButtonInfo().wasAnyButtonReleased()) {
		DN8App::get().getGUI().drawList(&this->MenuList);
	}
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType MenuState::onShutdown() {
	//MenuList.selectedItem = 0;
	DN8App::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

