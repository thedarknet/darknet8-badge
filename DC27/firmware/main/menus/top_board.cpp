#include "top_board.h"
#include "menu_state.h"
#include "../app.h"
#include "../buttons.h"
#include "calibration_menu.h"
#include "i2c.hpp"
#include <esp_log.h>
#include "gui_list_processor.h"

using libesp::ErrorType;
using libesp::RGBColor;
using libesp::Point2Ds;
using libesp::TouchNotification;
using libesp::ESP32_I2CMaster;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[TopBoardMenu::QUEUE_SIZE*TopBoardMenu::MSG_SIZE] = {0};
static const char *LOGTAG = "TopBoardMenu";

#define I2C_SCL2 GPIO_NUM_2
#define I2C_SDA2 GPIO_NUM_16
static ESP32_I2CMaster I2c1(I2C_SCL2,I2C_SDA2, 50000, I2C_NUM_1, 0, 16);


TopBoardMenu::TopBoardMenu() : DN8BaseMenu(), DeviceList("Devices:", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0, (sizeof(Items)/sizeof(Items[0]))), InternalQueueHandler() {
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

TopBoardMenu::~TopBoardMenu() {

}

bool TopBoardMenu::deviceInit() {
	ESP_LOGI(LOGTAG,"TopBoardMenu deviceInit");
	return I2c1.init(false);
}


ErrorType TopBoardMenu::onInit() {
	clearListBuffer();
	sprintf(getRow(8), "Exit release UP and DOWN");

	for(int i=0;i<(sizeof(Items)/sizeof(Items[0]));++i) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}

	int8_t found = 0;
	ErrorType et;
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:   ");
	for (uint8_t address = 1; address < 0x78; address++) {
		et = I2c1.probe(address);
		if (address % 16 == 0) {
			printf("\n%.2x:", address);
		}
		if (et.ok()) {
			printf(" %.2x", address);
			if(address==0x1c) {
				sprintf(getRow(found),"DarkNet Eyes: %.2x",address);
			} else {
				sprintf(getRow(found),"SAO: %.2x",address);
			}
			Items[found].id = address;
			++found;
		} else {
			printf(" --");
		}
	}
	printf("\n");

	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	DN8App::get().getTouch().addObserver(InternalQueueHandler);
	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext TopBoardMenu::onRun() {
	BaseMenu *nextState = this;

	TouchNotification *pe = nullptr;
	Point2Ds TouchPosInBuf;
	bool penUp = false;
	if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
		Point2Ds screenPoint(pe->getX(),pe->getY());
		TouchPosInBuf = DN8App::get().getCalibrationMenu()->getPickPoint(screenPoint);
		ESP_LOGI(LOGTAG,"TouchPoint: X:%d Y:%d PD:%d", int32_t(TouchPosInBuf.getX()),
							 int32_t(TouchPosInBuf.getY()), pe->isPenDown()?1:0);
		penUp = !pe->isPenDown();
		delete pe;
		int32_t touchGUI = GUIListProcessor::process(TouchPosInBuf, &DeviceList,(sizeof(Items) / sizeof(Items[0])));
		if(touchGUI==GUIListProcessor::NO_GUI_ITEM_HIT
				|| touchGUI==GUIListProcessor::GUI_HEADER_HIT) {
			pe = nullptr;
		}
	}
	if (pe || !GUIListProcessor::process(&DeviceList,(sizeof(Items)/sizeof(Items[0]))))
	{
		if (penUp || DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1))
		{
			switch (DeviceList.getSelectedItemID()) {
			case 0x1c: {
					uint8_t buf[2] = {1,1};
					if(I2c1.start(0x1c,true)) {
						if(I2c1.write(&buf[0],sizeof(buf),false)) {
							if(I2c1.stop(100)) {
								ESP_LOGI(LOGTAG,"update sent to top board");
							} else {
								ESP_LOGI(LOGTAG,"stop failed");
							}
						} else {
							ESP_LOGI(LOGTAG,"write failed");
						}
					} else {
						ESP_LOGI(LOGTAG,"start failed");
					}
				}
				break;
			default:
				break;
			}
		}
	}


	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	
	if (DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_RIGHT_DOWN| ButtonInfo::BUTTON_LEFT_UP)) {
		nextState = DN8App::get().getMenuState();
	}

	DN8App::get().getGUI().drawList(&DeviceList);
	return libesp::BaseMenu::ReturnStateContext(nextState);
}

ErrorType TopBoardMenu::onShutdown() {
	DN8App::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

