/*
 * gui_list_processor.cpp
 *
 *  Created on: Jul 16, 2018
 *      Author: dcomes
 */

#include "gui_list_processor.h"
#include <libesp/device/display/gui.h>
#include "../app.h"
#include "../buttons.h"

using namespace libesp;

bool GUIListProcessor::process(libesp::GUIListData *pl, uint16_t itemCount) {
	bool bHandled = false;
	if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_LEFT_UP)) {
		bHandled = true;
		do {
			if (pl->selectedItem == 0) {
				pl->selectedItem = itemCount - 1;
			} else {
				pl->selectedItem--;
			}
		} while(!pl->items[pl->selectedItem].text || pl->items[pl->selectedItem].text[0]=='\0');
	} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_RIGHT_DOWN)) {
		bHandled = true;
		do {
			if (pl->selectedItem == itemCount - 1) {
				pl->selectedItem = 0;
			} else {
				pl->selectedItem++;
			}
		} while(!pl->items[pl->selectedItem].text || pl->items[pl->selectedItem].text[0]=='\0');
	} 
	/*
	 * else if (DarkNet7::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DarkNet7::ButtonInfo::BUTTON_LEFT)) {
		bHandled = true;
		pl->selectedItem = 0;
	}
	*/
	return bHandled;
}



