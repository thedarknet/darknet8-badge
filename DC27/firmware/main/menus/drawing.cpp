#include "drawing.h"
#include "menu_state.h"
#include "../app.h"
#include "../buttons.h"
#include <math/point.h>
#include "calibration_menu.h"
#include "device/display/color.h"

using libesp::ErrorType;
using libesp::RGBColor;
using libesp::Point2Ds;
using libesp::TouchNotification;
using libesp::BaseMenu;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[DrawingMenu::QUEUE_SIZE*DrawingMenu::MSG_SIZE] = {0};
static const char *LOGTAG = "DrawingMenu";

static const RGBColor Colors[] = {
	RGBColor::BLUE,
	RGBColor::WHITE,
	RGBColor::RED,
	RGBColor::GREEN
};

static const uint32_t MAX_COLORS = sizeof(Colors)/sizeof(Colors[0]);

DrawingMenu::DrawingMenu() : DN8BaseMenu(), InternalQueueHandler(), ColorIndex(0) {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

DrawingMenu::~DrawingMenu() {

}


ErrorType DrawingMenu::onInit() {
	clearListBuffer();

	ColorIndex = 0;

	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	DN8App::get().getTouch().addObserver(InternalQueueHandler);
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	return ErrorType();
}
	
void DrawingMenu::drawVirtualButtons() {

}

BaseMenu *DrawingMenu::processVirtualButtons() {
	return this;
}

BaseMenu *DrawingMenu::processButtons() {
	return this;
}

libesp::BaseMenu::ReturnStateContext DrawingMenu::onRun() {
	BaseMenu *nextState = this;
	for(int i=0;i<10;++i) {
		TouchNotification *pe = nullptr;
		bool isPenDown = false;
		uint16_t penX = 0;
		uint16_t penY = 0;
		uint16_t penZ = 0;
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			penX = pe->getX();
			penY = pe->getY();
			penZ = pe->getZ();
			isPenDown = pe->isPenDown();
			delete pe;
			drawVirtualButtons();
			Point2Ds screenPoint(penX,penY);
			Point2Ds TouchPosInBuf = DN8App::get().getCalibrationMenu()->getPickPoint(screenPoint);
			sprintf(getRow(5), "Pen Pos: %d, %d", int32_t(TouchPosInBuf.getX()), int32_t(TouchPosInBuf.getY()));
			if(isPenDown) {
				if(TouchPosInBuf.getX()>10) {
					DN8App::get().getDisplay().drawPixel(TouchPosInBuf.getX(),TouchPosInBuf.getY(),Colors[ColorIndex]);
				}
			} else {
				if(TouchPosInBuf.getX()<10) {
					nextState = processVirtualButtons();
				}
			}
		}
	}
	if(nextState!=this) {
		nextState = processButtons();
	}
	
	return libesp::BaseMenu::ReturnStateContext(nextState);
}

ErrorType DrawingMenu::onShutdown() {
	DN8App::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

