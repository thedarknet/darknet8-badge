#include "test_menu.h"
#include "menu_state.h"
#include "../app.h"
#include "../buttons.h"
#include <math/point.h>
#include "calibration_menu.h"

using libesp::ErrorType;
using libesp::RGBColor;
using libesp::Point2Ds;
using libesp::TouchNotification;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[TestMenu::QUEUE_SIZE*TestMenu::MSG_SIZE] = {0};
static const char *LOGTAG = "TestMenu";

TestMenu::TestMenu() : DN8BaseMenu(), ButtonList("Button Info:", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0, ItemCount ), PenX(0), PenY(0), PenZ(0), IsPenDown(false), InternalQueueHandler() {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

TestMenu::~TestMenu() {

}


ErrorType TestMenu::onInit() {
	clearListBuffer();
	sprintf(getRow(8), "Exit release UP and DOWN");

	for (uint32_t i = 0; i < (sizeof(Items) / sizeof(Items[0])); i++) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	DN8App::get().getTouch().addObserver(InternalQueueHandler);
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext TestMenu::onRun() {
	BaseMenu *nextState = this;
	TouchNotification *pe = nullptr;
	bool hdrHit = false;
	bool update = false;
	if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
		PenX = pe->getX();
		PenY = pe->getY();
		PenZ = pe->getZ();
		IsPenDown = pe->isPenDown();
		update = true;
		delete pe;
	}

	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	sprintf(getRow(0), "LEFT/UP: %s", upAction()?DN8App::sYES : DN8App::sNO);
	sprintf(getRow(1), "RIGHT DOWN: %s", downAction()?DN8App::sYES : DN8App::sNO);
	sprintf(getRow(2), "FIRE1: %s", selectAction()?DN8App::sYES : DN8App::sNO);
	sprintf(getRow(3), "PenDown: %s", IsPenDown?DN8App::sYES:DN8App::sNO);
	if(update) {
		sprintf(getRow(4), "Pen Pos(raw): %d, %d %d", int32_t(PenX), int32_t(PenY), int32_t(PenZ));
		Point2Ds screenPoint(PenX,PenY);
		Point2Ds TouchPosInBuf = DN8App::get().getCalibrationMenu()->getPickPoint(screenPoint);
		if(TouchPosInBuf.getY()<=10 && !IsPenDown) {
			hdrHit = true;
		}
		sprintf(getRow(5), "Pen Pos: %d, %d", int32_t(TouchPosInBuf.getX()), int32_t(TouchPosInBuf.getY()));
	}
	
	if (backAction() || hdrHit) {
		nextState = DN8App::get().getMenuState();
	}

	DN8App::get().getGUI().drawList(&ButtonList);
	return libesp::BaseMenu::ReturnStateContext(nextState);
}

ErrorType TestMenu::onShutdown() {
	DN8App::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

