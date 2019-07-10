#include "menu_state.h"
#include "../app.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

MenuState::MenuState() :
		DN8BaseMenu(), MenuList("Main Menu", Items, 0, 0, DN8App::get().getCanvasWidth(),
				DN8App::get().getCanvasHeight(), 0, (sizeof(Items) / sizeof(Items[0]))) {
}

MenuState::~MenuState() {

}


ErrorType MenuState::onInit() {
	Items[0].id = 0;
	//if (DarkNet7::get().getContacts().getSettings().isNameSet()) {
		Items[0].text = (const char *) "Settings";
//	} else {
//		Items[0].text = (const char *) "Settings *";
//	}
	Items[1].id = 1;
	Items[1].text = (const char *) "Badge Pair";
	Items[2].id = 2;
	Items[2].text = (const char *) "Address Book";
	Items[3].id = 3;
	Items[3].text = (const char *) "3D";
	Items[4].id = 4;
	Items[4].text = (const char *) "Screen Saver";
	Items[5].id = 5;
	Items[5].text = (const char *) "MCU Info";
	Items[6].id = 6;
	Items[6].text = (const char *) "Communications Settings";
	Items[7].id = 7;
	Items[7].text = (const char *) "Health";
	Items[8].id = 8;
	Items[8].text = (const char *) "Scan for NPCs";
	Items[9].id = 9;
	Items[9].text = (const char *) "Test Badge";
	Items[10].id = 10;
	Items[10].text = (const char *) "Scan: Shitty Addon Badge";
	Items[11].id = 11;
	Items[11].text = (const char *) "Calibrate Touch";
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawList(&this->MenuList);
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext MenuState::onRun() {
	BaseMenu *nextState = this;
/*
	if (!GUIListProcessor::process(&MenuList,(sizeof(Items) / sizeof(Items[0]))))
	{
		if (DarkNet7::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DarkNet7::ButtonInfo::BUTTON_FIRE1))
		{
			switch (MenuList.selectedItem)
			{
				case 0:
					nextState = DarkNet7::get().getSettingState();
					break;
				case 1:
					if (DarkNet7::get().getContacts().getSettings().getAgentName()[0] != '\0') {
						nextState = DarkNet7::get().getPairingState();
					} else {
						nextState = DarkNet7::get().getDisplayMessageState(DarkNet7::get().getDisplayMenuState(),
								(const char *) "You must set your agent name first", 3000);
					}
					break;
				case 2:
					nextState = DarkNet7::get().getAddressBookState();
					break;
				case 3:
					nextState = DarkNet7::get().get3DState();
					break;
				case 4:
					nextState = DarkNet7::get().getGameOfLifeState();
					break;
				case 5:
					nextState = DarkNet7::get().getBadgeInfoState();
					break;
				case 6:
					nextState = DarkNet7::get().getMCUInfoState();
					break;
				case 7:
					nextState = DarkNet7::get().getCommunicationSettingState();
					break;
				case 8:
					nextState = DarkNet7::get().getHealthState();
					break;
				case 9:
					DarkNet7::get().getScanState()->setNPCOnly(true);
					nextState = DarkNet7::get().getScanState();
					break;
				case 10:
					nextState = DarkNet7::get().getTestState();
					break;
				case 11:
					nextState = DarkNet7::get().getSAOMenuState();
					break;

			}
		}
	}

	if (DarkNet7::get().getButtonInfo().wasAnyButtonReleased()) {
		DarkNet7::get().getGUI().drawList(&this->MenuList);
	}
*/
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType MenuState::onShutdown() {
	//MenuList.selectedItem = 0;
	return ErrorType();
}
