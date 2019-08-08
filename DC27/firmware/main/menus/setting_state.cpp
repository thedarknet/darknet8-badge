/*
 * setting_state.cpp
 *
 *      Author: cmdc0de
 */

#include "setting_state.h"
#include <device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include "../buttons.h"
#include "../wifi.h"
#include <app/display_message_state.h>
#include <device/touch/XPT2046.h>
#include "gui_list_processor.h"
#include <esp_log.h>
#include "drawing.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::TouchNotification;

static StaticQueue_t TouchQueue;
static uint8_t TouchQueueBuffer[SettingMenu::TOUCH_QUEUE_SIZE*SettingMenu::TOUCH_MSG_SIZE] = {0};
const char *SettingMenu::LOGTAG = "SettingMenu";

SettingMenu::SettingMenu() 
	: DN8BaseMenu(), SettingList((const char *) "MENU", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel()-4, DN8App::get().getLastCanvasHeightPixel(), 0, ItemCount), AgentName(), SubState(0), MiscCounter(0), VKB(), IHC(&AgentName[0],sizeof(AgentName)) {

	TouchQueueHandle = xQueueCreateStatic(TOUCH_QUEUE_SIZE,TOUCH_MSG_SIZE,&TouchQueueBuffer[0],&TouchQueue);
}

SettingMenu::~SettingMenu() {

}

ErrorType SettingMenu::onInit() {
	SubState = 0;
	MiscCounter = 0;
	memset(&AgentName[0], 0, sizeof(AgentName));
	Items[0].id = 0;
	Items[0].text = (const char *) "Set Agent Name";
	Items[1].id = 1;
	Items[1].text = (const char *) "Screen Saver Time";
	Items[1].setShouldScroll();
	Items[2].id = 2;
	Items[2].text = (const char *) "Features";
	Items[3].id = 3;
	Items[3].text = (const char *) "Factory Reset";
	Items[3].setShouldScroll();
	Items[4].id = 4;
	Items[4].text = (const char *) "OTA Update";
	Items[4].setShouldScroll();
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawList(&SettingList);
	IHC.set(&AgentName[0],sizeof(AgentName));
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(TouchQueueHandle, &pe, 0)) {
			delete pe;
		}
	}
	DN8App::get().getTouch().addObserver(TouchQueueHandle);
	return ErrorType();
}

static char Misc[8] = {'\0'};

BaseMenu::ReturnStateContext SettingMenu::onRun() {
	BaseMenu *nextState = this;
	TouchNotification *pe = nullptr;
	bool penUp = false;
	bool hdrHit = false;
	pe = processTouch(TouchQueueHandle, SettingList, ItemCount, penUp, hdrHit);
	if (0 == SubState) {
		if (pe || !GUIListProcessor::process(&SettingList,ItemCount)) {
			if (penUp || selectAction()) {
				SubState = SettingList.selectedItem + 100;
				DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
				switch (SubState) {
				case 100:
					memset(&AgentName[0], 0, sizeof(AgentName));
					VKB.init(VirtualKeyBoard::STDKBNames, &IHC, 5, DN8App::get().getLastCanvasWidthPixel()-5, 80, libesp::RGBColor::WHITE, RGBColor::BLACK, RGBColor::BLUE, '_');
					DN8App::get().getDisplay().drawString(0, 10, (const char*) "Current agent name:");
					if (*DN8App::get().getContacts().getSettings().getAgentName()	== '\0') {
						DN8App::get().getDisplay().drawString(0, 20, (const char *) "NOT SET");
					} else {
						DN8App::get().getDisplay().drawString(0, 20, DN8App::get().getContacts().getSettings().getAgentName());
					}
					DN8App::get().getDisplay().drawString(0, 40, (const char*) "Release UP/DOWN buttons completes entry");
					DN8App::get().getDisplay().drawString(0, 50, (const char*) "Set agent name:");
					DN8App::get().getDisplay().drawString(0, 60, &AgentName[0]);
					break;
				case 101:
					MiscCounter = DN8App::get().getContacts().getSettings().getScreenSaverTime();
					break;
				case 102:
					MiscCounter = DN8App::get().getContacts().getSettings().isBLE()?1:0;
					ESP_LOGI(LOGTAG,"MiscCounter %d",MiscCounter);
					break;
				case 103:
					DN8App::get().getDisplay().drawString(0, 10, (const char*) "ERASE ALL\nCONTACTS?");
					DN8App::get().getDisplay().drawString(0, 30, (const char*) "Fire1 = YES");
					break;
				}
			} else if (backAction() || hdrHit) {
				nextState = DN8App::get().getMenuState();
			}
		}
	} else {
		switch (SubState) {
		case 100:
			VKB.process();
			if ((hdrHit||backAction()) && AgentName[0] != '\0' && AgentName[0] != ' ' && AgentName[0] != '_') {
				AgentName[Contact::AGENT_NAME_LENGTH - 1] = '\0';
				if (DN8App::get().getContacts().getSettings().setAgentname(&AgentName[0])) {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Save Successful", 2000);
				} else {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Save FAILED!",	4000);
				}
			} else {
				DN8App::get().getDisplay().drawString(0, 60, &AgentName[0]);
			}
			break;
		case 101: {
			if(MiscCounter>9) MiscCounter=9;
			else if (MiscCounter<1) MiscCounter=1;
			sprintf(&Misc[0],"%d",(int)MiscCounter);
			DN8App::get().getDisplay().drawString(0, 10, (const char*) "Badge Sleep Time:", RGBColor::WHITE, RGBColor::BLACK, 1, true);
			DN8App::get().getDisplay().drawString(0, 30, (const char*) "Up to increase, down to decrease", RGBColor::WHITE, RGBColor::BLACK, 1, true);
			DN8App::get().getDisplay().drawString(10, 60, &Misc[0], RGBColor::WHITE, RGBColor::BLACK, 1, true);
			DN8App::get().getDisplay().drawString(0, 100, (const char*) "Fire Button completes", RGBColor::WHITE, RGBColor::BLACK, 1, true);
			if (upAction()) {
				MiscCounter++;
			} else if (downAction()) {
				MiscCounter--;
			} else if (selectAction()) {
				if (DN8App::get().getContacts().getSettings().setScreenSaverTime(MiscCounter)) {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Setting saved", 2000);
				} else {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Save FAILED!", 4000);
				}
			}
		}
			break;
		case 102:
			if(MiscCounter==0) {
				DN8App::get().getDisplay().drawString(0, 30, (const char*) "BLE: YES WIFI NO", RGBColor::WHITE, RGBColor::BLACK, 1, true);
				DN8App::get().getDisplay().drawString(0, 40, (const char*) "BLE: NO WIFI YES", RGBColor::BLACK, RGBColor::WHITE, 1, true);
			} else {
				DN8App::get().getDisplay().drawString(0, 40, (const char*) "BLE: YES WIFI NO", RGBColor::BLACK, RGBColor::WHITE, 1, true);
				DN8App::get().getDisplay().drawString(0, 30, (const char*) "BLE: NO  WIFI YES", RGBColor::WHITE, RGBColor::BLACK, 1, true);
			}
			DN8App::get().getDisplay().drawString(0,80,(const char*)"Fire Completes");
			if (upAction() || downAction()) {
				MiscCounter = MiscCounter==0?1:0;
				ESP_LOGI(LOGTAG,"mc:%d",MiscCounter);
			} else if (selectAction()) {
				if(DN8App::get().getContacts().getSettings().setBLE(MiscCounter==1)) {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Setting saved", 2000);
				} else {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Save FAILED!", 4000);
				}
			}
			break;
		case 103:
			if(selectAction()) {
				DN8App::get().getContacts().resetToFactory();
				DN8App::get().getDrawingMenu()->clearStorage();
				nextState = DN8App::get().getMenuState();
			} else if (DN8App::get().getButtonInfo().wasAnyButtonReleased()) {
				nextState = DN8App::get().getMenuState();
			}
			break;
		case 104:
			uint32_t nvsh;
			nvs_open("nvs", NVS_READWRITE, &nvsh);
			nvs_set_i32(nvsh, "do_ota", 1);
			nvs_commit(nvsh);
			DN8App::get().getDisplay().drawString(0, 10,
				(const char*) "Power Cycle Badge");
			while (true);
			break;
		}
	}
	if(SubState<100 && (penUp|| DN8App::get().getButtonInfo().wasAnyButtonReleased())) {
		DN8App::get().getGUI().drawList(&SettingList);
	}
	return ReturnStateContext(nextState);
}

ErrorType SettingMenu::onShutdown() {
	DN8App::get().getTouch().removeObserver(TouchQueueHandle);
	return ErrorType();
}
