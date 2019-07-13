/*
 *
 *      Author: libesp
 */

#include "badge_info_menu.h"
#include "menu_state.h"
#include "gui_list_processor.h"

using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;

BadgeInfoMenu::BadgeInfoMenu() : DN8BaseMenu(), 
	BadgeInfoList("Badge Info:", Items, 0, 0, DN8App::getCanvasLastWidthPixel(), DN8App::getCanvasLastHeightPixel(),
				0, (sizeof(Items) / sizeof(Items[0]))), RegCode() {
	memset(&RegCode, 0, sizeof(RegCode));
}

BadgeInfoMenu::~BadgeInfoMenu() {

}

const char *BadgeInfoMenu::getRegCode(ContactStore &cs) {
/*
	if (RegCode[0] == 0) {
		ShaOBJ hashObj;
		sha256_init(&hashObj);
		sha256_add(&hashObj, cs.getMyInfo().getPrivateKey(),
				ContactStore::PRIVATE_KEY_LENGTH);
		uint16_t id = cs.getMyInfo().getUniqueID();
		sha256_add(&hashObj, (uint8_t *) &id, sizeof(id));
		uint8_t rH[SHA256_HASH_SIZE];
		sha256_digest(&hashObj, &rH[0]);
		sprintf(&RegCode[0], "%02x%02x%02x%02x%02x%02x%02x%02x", rH[0], rH[1],
				rH[2], rH[3], rH[4], rH[5], rH[6], rH[7]);
	}
*/
	return &RegCode[0];
}

static const char *VERSION = "8.dc27.1";

ErrorType BadgeInfoMenu::onInit() {
	memset(&ListBuffer[0], 0, sizeof(ListBuffer));
	sprintf(&ListBuffer[0][0], "Name: %s", "NOT DONE"); //DN8App::get().getContacts().getSettings().getAgentName());
	sprintf(&ListBuffer[1][0], "Num contacts: %u", 0); //DN8App::get().getContacts().getSettings().getNumContacts());
	sprintf(&ListBuffer[2][0], "REG: %s", "NOT DONE"); //getRegCode(DN8App::get().getContacts()));
	sprintf(&ListBuffer[3][0], "UID: %u", 0); //DN8App::get().getContacts().getMyInfo().getUniqueID());
	uint8_t fake[24] = {1};
	uint8_t *pCP =	&fake[0]; //DN8App::get().getContacts().getMyInfo().getCompressedPublicKey();
	sprintf(&ListBuffer[4][0], "PK: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			pCP[0], pCP[1], pCP[2], pCP[3], pCP[4], pCP[5], pCP[6], pCP[7],
			pCP[8], pCP[9], pCP[10], pCP[11], pCP[12], pCP[13], pCP[14],
			pCP[15], pCP[16], pCP[17], pCP[18], pCP[19], pCP[20], pCP[21],
			pCP[22], pCP[23], pCP[24]);
	sprintf(&ListBuffer[5][0], "Free HeapSize: %u\n",libesp::System::getFreeHeapSize());
	sprintf(&ListBuffer[6][0], "Free Min HeapSize: %u\n",libesp::System::getMinimumFreeHeapSize());
	sprintf(&ListBuffer[7][0], "HAL Version: %s", libesp::System::getIDFVersion());
	sprintf(&ListBuffer[8][0], "SVer: %s", VERSION);

	for (uint32_t i = 0; i < (sizeof(Items) / sizeof(Items[0])); i++) {
		Items[i].text = &ListBuffer[i][0];
		Items[i].id = i;
		Items[i].setShouldScroll();
	}
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawList(&BadgeInfoList);
	return ErrorType();
}

BaseMenu::ReturnStateContext BadgeInfoMenu::onRun() {

	BaseMenu *nextState = this;
	if(!GUIListProcessor::process(&BadgeInfoList,BadgeInfoList.ItemsCount)) {
		if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_RIGHT_DOWN|
						ButtonInfo::BUTTON_LEFT_UP)) {
			nextState = DN8App::get().getDisplayMenuState();
		}
	}
	return ReturnStateContext(nextState);
}

ErrorType BadgeInfoMenu::onShutdown() {
	return ErrorType();
}

