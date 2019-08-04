
#include "dn8base_menu.h"
#include "device/touch/XPT2046.h"
#include "math/point.h"
#include "../app.h"
#include "calibration_menu.h"
#include "gui_list_processor.h"
#include <esp_log.h>
#include "../buttons.h"

using libesp::TouchNotification;
using libesp::Point2Ds;

const char *DN8BaseMenu::LOGTAG = "DN8BaseMenu";

char DN8BaseMenu::ListBuffer[10][32] = {0};
uint8_t DN8BaseMenu::NumRows = 10;

void DN8BaseMenu::clearListBuffer() {
	memset(&ListBuffer[0], 0, sizeof(ListBuffer));
}

char *DN8BaseMenu::getRow(uint8_t row) {
	if(row>=NumRows) return nullptr;
	return &ListBuffer[row][0];
}

TouchNotification *DN8BaseMenu::processTouch(QueueHandle_t &queH, libesp::GUIListData &guiList, uint16_t itemCount, bool &penUp, bool &headerHit) {
	TouchNotification *pe = nullptr;
	Point2Ds TouchPosInBuf;
	headerHit = false;
	if(xQueueReceive(queH, &pe, 0)) {
		ESP_LOGI(LOGTAG,"que");
		Point2Ds screenPoint(pe->getX(),pe->getY());
		TouchPosInBuf = DN8App::get().getCalibrationMenu()->getPickPoint(screenPoint);
		ESP_LOGI(LOGTAG,"TouchPoint: X:%d Y:%d PD:%d", int32_t(TouchPosInBuf.getX()),
							 int32_t(TouchPosInBuf.getY()), pe->isPenDown()?1:0);
		penUp = !pe->isPenDown();
		delete pe;
		int32_t touchGUI = GUIListProcessor::process(TouchPosInBuf, &guiList,itemCount);
		if(touchGUI==GUIListProcessor::NO_GUI_ITEM_HIT
				|| touchGUI==GUIListProcessor::GUI_HEADER_HIT) {
			pe = nullptr;
		}
		if(touchGUI==GUIListProcessor::GUI_HEADER_HIT) headerHit = true;
		ESP_LOGI(LOGTAG,"pe: %d", int32_t(pe));
	}
	return pe;
}

bool DN8BaseMenu::backAction() {
	return DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_LEFT_UP|ButtonInfo::BUTTON_RIGHT_DOWN);
}

bool DN8BaseMenu::selectAction() {
	return DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1);
}

bool DN8BaseMenu::upAction() {
	return DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_LEFT_UP);
}

bool DN8BaseMenu::downAction() {
	return DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_RIGHT_DOWN);
}

