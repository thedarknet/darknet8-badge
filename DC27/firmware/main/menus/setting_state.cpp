/*
 * setting_state.cpp
 *
 *      Author: libesp
 */

#include "setting_state.h"
#include <device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include "../buttons.h"
#include <app/display_message_state.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

SettingMenu::SettingMenu() : DN8BaseMenu(), SettingList((const char *) "MENU", Items, 0, 0,
		DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0, sizeof(Items) / sizeof(Items[0])), AgentName(), SubState(0), MiscCounter(0), VKB(), IHC(&AgentName[0],sizeof(AgentName)) {

	memset(&AgentName[0], 0, sizeof(AgentName));
	Items[0].id = 0;
	Items[0].text = (const char *) "Set Agent Name";
	Items[1].id = 1;
	Items[1].text = (const char *) "Screen Saver Time";
	Items[1].setShouldScroll();
	Items[2].id = 2;
	Items[2].text = (const char *) "Reset Badge Contacts";
	Items[2].setShouldScroll();
}

SettingMenu::~SettingMenu() {

}

ErrorType SettingMenu::onInit() {
	SubState = 0;
	MiscCounter = 0;
	memset(&AgentName[0], 0, sizeof(AgentName));
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawList(&SettingList);
	IHC.set(&AgentName[0],sizeof(AgentName));
	return ErrorType();
}

static char Misc[8] = {'\0'};

BaseMenu::ReturnStateContext SettingMenu::onRun() {
	BaseMenu *nextState = this;
	if (0 == SubState) {
		if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_LEFT_UP)) {
			if (SettingList.selectedItem == 0) {
				SettingList.selectedItem = sizeof(Items) / sizeof(Items[0]) - 1;
			} else {
				SettingList.selectedItem--;
			}
		} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_RIGHT_DOWN)) {
			if (SettingList.selectedItem
					== (sizeof(Items) / sizeof(Items[0]) - 1)) {
				SettingList.selectedItem = 0;
			} else {
				SettingList.selectedItem++;
			}
		} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1)) {
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
				DN8App::get().getDisplay().drawString(0, 40, (const char*) "MID button completes entry");
				DN8App::get().getDisplay().drawString(0, 50, (const char*) "Set agent name:");
				DN8App::get().getDisplay().drawString(0, 60, &AgentName[0]);
				break;
			case 101:
				MiscCounter = DN8App::get().getContacts().getSettings().getScreenSaverTime();
				break;
			case 102:
				DN8App::get().getDisplay().drawString(0, 10, (const char*) "ERASE ALL\nCONTACTS?");
				DN8App::get().getDisplay().drawString(0, 30, (const char*) "Fire1 = YES");
				break;
			}
		}
	} else {
		switch (SubState) {
		case 100:
			VKB.process();
			if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1) && AgentName[0] != '\0' && AgentName[0] != ' ' && AgentName[0] != '_') {
				AgentName[ContactStore::AGENT_NAME_LENGTH - 1] = '\0';
				if (DN8App::get().getContacts().getSettings().setAgentname(&AgentName[0])) {
					nextState = DN8App::get().getDisplayMessageState(	DN8App::get().getMenuState(), (const char *)"Save Successful", 2000);
				} else {
					nextState = DN8App::get().getDisplayMessageState(	DN8App::get().getMenuState(), (const char *)"Save FAILED!",	4000);
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
				DN8App::get().getDisplay().drawString(0, 100, (const char*) "MID Button completes", RGBColor::WHITE, RGBColor::BLACK, 1, true);
				if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_LEFT_UP)) {
					MiscCounter++;
				} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_RIGHT_DOWN)) {
					MiscCounter--;
				} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1)) {
					if (DN8App::get().getContacts().getSettings().setScreenSaverTime(MiscCounter)) {
						nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Setting saved", 2000);
					} else {
						nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(), (const char *)"Save FAILED!", 4000);
					}
				}
			}
			break;
		case 102:
			if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1)) {
				DN8App::get().getContacts().resetToFactory();
				nextState = DN8App::get().getMenuState();
			} else if (DN8App::get().getButtonInfo().wasAnyButtonReleased()) {
				nextState = DN8App::get().getMenuState();
			}
			break;
		}
	}
	if (SubState < 100 && DN8App::get().getButtonInfo().wasAnyButtonReleased()) {
		DN8App::get().getGUI().drawList(&SettingList);
	}
	return ReturnStateContext(nextState);
}

ErrorType SettingMenu::onShutdown() {
	return ErrorType();
}
