#include "test_menu.h"
#include "menu_state.h"
#include "../app.h"
#include "../buttons.h"

using libesp::ErrorType;
using libesp::RGBColor;


TestMenu::TestMenu() : DN8BaseMenu(), ButtonList("Button Info:", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0, (sizeof(Items) / sizeof(Items[0]))), TimesMidHasBeenHeld(0) {

}

TestMenu::~TestMenu() {

}


ErrorType TestMenu::onInit() {
	TimesMidHasBeenHeld=0;
	clearListBuffer();
	sprintf(getRow(5), "Exit hold FIRE");

	for (uint32_t i = 0; i < (sizeof(Items) / sizeof(Items[0])); i++) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext TestMenu::onRun() {
	BaseMenu *nextState = this;

	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	sprintf(getRow(0), " LEFT/UP: %s", DN8App::get().getButtonInfo().isAnyOfTheseButtonDown(ButtonInfo::BUTTON_LEFT_UP)?DN8App::sYES : DN8App::sNO);
	sprintf(getRow(1), " RIGHT DOWN: %s", DN8App::get().getButtonInfo().isAnyOfTheseButtonDown(ButtonInfo::BUTTON_RIGHT_DOWN)?DN8App::sYES : DN8App::sNO);
	sprintf(getRow(2), "FIRE1: %s", DN8App::get().getButtonInfo().isAnyOfTheseButtonDown(ButtonInfo::BUTTON_FIRE1)?DN8App::sYES : DN8App::sNO);
	if(DN8App::get().getButtonInfo().isAnyOfTheseButtonDown(ButtonInfo::BUTTON_FIRE1)) {
		TimesMidHasBeenHeld++;
	} else {
		TimesMidHasBeenHeld=0;
	}
	if(TimesMidHasBeenHeld>EXIT_COUNT) {
		nextState = DN8App::get().getMenuState();
	}

	DN8App::get().getGUI().drawList(&ButtonList);
	return libesp::BaseMenu::ReturnStateContext(nextState);
}

ErrorType TestMenu::onShutdown() {
	return ErrorType();
}

