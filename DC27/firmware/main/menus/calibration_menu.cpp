#include "calibration_menu.h"
#include "menu_state.h"
#include "../app.h"
#include <libesp/math/point.h>
#include <libesp/device/display/display_device.h>
#include "../buttons.h"
#include <esp_log.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::XPT2046;

const char *CalibrationMenu::LOGTAG = "CalibrationMenu";
static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[CalibrationMenu::QUEUE_SIZE*CalibrationMenu::MSG_SIZE] = {0};

CalibrationMenu::CalibrationMenu() : DN8BaseMenu(), CalibrationLocations(), CurrentIndex(TOTAL), CalibrationData("nvs","cd", false) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

ErrorType CalibrationMenu::loadCalibrationData() {
	size_t size = sizeof(CalibrationLocations);
	ErrorType et = CalibrationData.getBlob("caldata",&CalibrationLocations[0],size);
	if(et==ESP_ERR_NVS_NOT_FOUND) {
		et = ErrorType();
		ESP_LOGI(LOGTAG,"No Calibration Data");
	} else if(et.ok()) {
		ESP_LOGI(LOGTAG,"Loaded Calibration Data");
		for(int i=0;i<TOTAL;i++) {
			ESP_LOGI(LOGTAG,"POS: %d, X: %d Y: %d",i,
			(int)CalibrationLocations[i].getX(), 
			(int)CalibrationLocations[i].getY()); 
		}
	}
	return et;
}

ErrorType CalibrationMenu::initNVS() {
	ErrorType et = CalibrationData.init();
	if(et.ok()) {
		CalibrationData.logInfo();
		et = loadCalibrationData();
	}
	return et;
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
	uint16_t x = 0,y = 0;
	switch(CurrentIndex) {
	case MID:
		x = DN8App::get().getCanvasWidth()/2;
		y = DN8App::get().getCanvasHeight()/2;
		DN8App::get().getDisplay().drawVerticalLine(x, y-4, 8 , RGBColor::RED);
		DN8App::get().getDisplay().drawHorizontalLine(x-3, y, 8 , RGBColor::RED);
		break;
	case TOP_LEFT:
		x = 0;
		y = 0;
		DN8App::get().getDisplay().drawVerticalLine(0, 0, 5 , RGBColor::RED);
		DN8App::get().getDisplay().drawHorizontalLine(0, 0, 5 , RGBColor::RED);
		break;
	case TOP_RIGHT:
		x = DN8App::get().getLastCanvasWidthPixel();
		y = 0;
		DN8App::get().getDisplay().drawVerticalLine(x, y, 5 , RGBColor::RED);
		DN8App::get().getDisplay().drawHorizontalLine(x-5, y, 5 , RGBColor::RED);
		break;
	case BOTTOM_LEFT:
		x = 0;
		y = DN8App::get().getLastCanvasHeightPixel();
		DN8App::get().getDisplay().drawVerticalLine(x, y-5, 5 , RGBColor::RED);
		DN8App::get().getDisplay().drawHorizontalLine(x, y, 5 , RGBColor::RED);
		break;
	case BOTTOM_RIGHT:
		x = DN8App::get().getLastCanvasWidthPixel();
		y = DN8App::get().getLastCanvasHeightPixel();
		DN8App::get().getDisplay().drawVerticalLine(x-1, y-5, 5 , RGBColor::RED);
		DN8App::get().getDisplay().drawHorizontalLine(x-4, y-2, 5 , RGBColor::RED);
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
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
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
		ErrorType et = CalibrationData.setBlob("caldata",&CalibrationLocations[0],sizeof(CalibrationLocations));
		if(et.ok()) {
			ESP_LOGI(LOGTAG,"Calibation data saved successfully!");
		}
		nextState = DN8App::get().getMenuState();
	}
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType CalibrationMenu::onShutdown() {
	//MenuList.selectedItem = 0;
	DN8App::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

