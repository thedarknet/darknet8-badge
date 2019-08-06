/*
 *
 *      Author: libesp
 */

#include "badge_info_menu.h"
#include "menu_state.h"
#include "gui_list_processor.h"
#include "../app.h"
#include "../buttons.h"
#include <system.h>
#include "cryptoauthlib.h"
#include "../KeyStore.h"

using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;

BadgeInfoMenu::BadgeInfoMenu() : DN8BaseMenu(), 
	BadgeInfoList("Badge Info:", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(),
				0, (sizeof(Items) / sizeof(Items[0]))), RegCode() {
	memset(&RegCode, 0, sizeof(RegCode));
}

BadgeInfoMenu::~BadgeInfoMenu() {

}

const char *BadgeInfoMenu::getRegCode() {
	if (RegCode[0] == 0) {
		uint8_t rH[9];
		ATCA_STATUS status = atcab_read_serial_number(&rH[0]);
		sprintf(&RegCode[0], "%02x%02x%02x%02x%02x%02x%02x%02x%02x", rH[0], rH[1],
				rH[2], rH[3], rH[4], rH[5], rH[6], rH[7], rH[8]);
	}
	return &RegCode[0];
}

static const char *VERSION = "8.dc27.1";

ErrorType BadgeInfoMenu::onInit() {
	clearListBuffer();
	sprintf(getRow(0), "Name: %s", DN8App::get().getContacts().getSettings().getAgentName());
	sprintf(getRow(1), "Num contacts: %u", DN8App::get().getContacts().getNumContacts());
	sprintf(getRow(2), "REG: %s", getRegCode());
	
	sprintf(getRow(3), "FrameBuffer %d KB", (DN8App::get().getBackBufferSize()/1024));
	const uint8_t *pCP =	DN8App::get().getContacts().getMyInfo().getPublicKey();
	sprintf(getRow(4), "PK: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			pCP[0], pCP[1], pCP[2], pCP[3], pCP[4], pCP[5], pCP[6], pCP[7],
			pCP[8], pCP[9], pCP[10], pCP[11], pCP[12], pCP[13], pCP[14],
			pCP[15], pCP[16], pCP[17], pCP[18], pCP[19], pCP[20], pCP[21],
			pCP[22], pCP[23], pCP[24]);
	sprintf(getRow(5), "Free HeapSize: %u\n",libesp::System::get().getFreeHeapSize());
	sprintf(getRow(6), "Free Min HeapSize: %u\n",libesp::System::get().getMinimumFreeHeapSize());
	sprintf(getRow(7), "IDF Version: %s", libesp::System::get().getIDFVersion());
	sprintf(getRow(8), "SVer: %s", VERSION);

	for (uint32_t i = 0; i < (sizeof(Items) / sizeof(Items[0])); i++) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getGUI().drawList(&BadgeInfoList);
	return ErrorType();
}

BaseMenu::ReturnStateContext BadgeInfoMenu::onRun() {
	BaseMenu *nextState = this;
	if (backAction() || selectAction()) {
		nextState = DN8App::get().getMenuState();
	} else if(GUIListProcessor::process(&BadgeInfoList,BadgeInfoList.ItemsCount)) {
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
		DN8App::get().getGUI().drawList(&BadgeInfoList);
	}
	return ReturnStateContext(nextState);
}

ErrorType BadgeInfoMenu::onShutdown() {
	return ErrorType();
}

