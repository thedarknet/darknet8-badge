#include "calibration_menu.h"
#include "menu_state.h"
#include "../app.h"
#include <libesp/math/point.h>
#include <libesp/device/display/display_device.h>
#include "../buttons.h"
#include "gui_list_processor.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;

CalibrationMenu::CalibrationMenu() : DN8BaseMenu(), CalibrationLocations() {
}

CalibrationMenu::~CalibrationMenu() {

}

ErrorType CalibrationMenu::onInit() {
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getDisplay().drawString(10,30, "Hello");
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext CalibrationMenu::onRun() {
	BaseMenu *nextState = this;

	if (DN8App::get().getButtonInfo().wasAnyButtonReleased()) {
		nextState = DN8App::get().getMenuState();
	}
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType CalibrationMenu::onShutdown() {
	//MenuList.selectedItem = 0;
	return ErrorType();
}

