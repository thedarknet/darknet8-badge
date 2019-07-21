#include "calibration_menu.h"
#include "menu_state.h"
#include "../app.h"
#include <libesp/math/point.h>
#include <libesp/device/display/display_device.h>
#include "../buttons.h"
#include <esp_log.h>
//#include "gui_list_processor.h"

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::XPT2046;

const char *CalibrationMenu::LOGTAG = "CalibrationMenu";
static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[CalibrationMenu::QUEUE_SIZE*CalibrationMenu::MSG_SIZE] = {0};

CalibrationMenu::CalibrationMenu() : DN8BaseMenu(), CalibrationLocations(), CurrentIndex(TOTAL) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

CalibrationMenu::~CalibrationMenu() {

}

ErrorType CalibrationMenu::onInit() {
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	CurrentIndex = MID;
	//empty queue
	XPT2046::TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	DN8App::get().getTouch().addObserver(InternalQueueHandler);
	return ErrorType();
}

void CalibrationMenu::drawCrossHairs() {
	uint16_t x,y;
	switch(CurrentIndex) {
	case MID:
		x = DN8App::get().getCanvasWidth()/2;
		y = DN8App::get().getCanvasHeight()/2;
		break;
	case TOP_LEFT:
		x = 0;
		y = 0;
		break;
	case TOP_RIGHT:
		x = DN8App::get().getCanvasWidth();
		y = 0;
		break;
	case BOTTOM_LEFT:
		x = 0;
		y = DN8App::get().getCanvasHeight();
		break;
	case BOTTOM_RIGHT:
		x = DN8App::get().getCanvasWidth();
		y = DN8App::get().getCanvasHeight();
		break;
	default:
		break;
	}
}

libesp::BaseMenu::ReturnStateContext CalibrationMenu::onRun() {
	BaseMenu *nextState = this;

	drawCrossHairs();
	XPT2046::TouchNotification *pe = nullptr;
	if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
		CalibrationLocations[CurrentIndex] = libesp::Point2Ds(pe->getX(), pe->getY());
		ESP_LOGI(LOGTAG,"CurrentIndex %d, x: %d, y:%d", (int32_t)CurrentIndex,
				(int32_t)CalibrationLocations[CurrentIndex].getX(),
				(int32_t)CalibrationLocations[CurrentIndex].getY());
		CurrentIndex++;	
		delete pe;
	}

	if (DN8App::get().getButtonInfo().wasAnyButtonReleased()) {
		nextState = DN8App::get().getMenuState();
	} else if(CurrentIndex>=TOTAL) {
		//save array
		nextState = DN8App::get().getMenuState();
	}
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType CalibrationMenu::onShutdown() {
	//MenuList.selectedItem = 0;
	DN8App::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

