/*
 * app.cpp
 *
 * Author: cmdc0de
 */

#include "app.h"
#include <esp_log.h>
#include <system.h>
#include <spibus.h>
#include <device/display/frame_buffer.h>
#include <device/display/display_device.h>
#include <device/display/fonts.h>
#include <device/display/gui.h>
#include <device/touch/XPT2046.h>

#include "./ble.h"
#include "./game_master.h"
#include "./wifi.h"
#include "menus/menu_state.h"
#include "buttons.h"
#include "menus/calibration_menu.h"
#include "KeyStore.h"
#include <app/display_message_state.h>
#include "menus/communications_settings.h"
#include "menus/badge_info_menu.h"
#include "menus/game_of_life.h"
#include "menus/pairing_menu.h"
#include "menus/scan.h"
#include "menus/setting_state.h"
#include "menus/test_menu.h"
#include "menus/menu3d.h"
#include "menus/top_board.h"
#include <driver/uart.h>

// This contains GPIO Pin definitions for various kits
#include "devkit.h"

using libesp::ErrorType;
using libesp::DisplayILI9341;
using libesp::XPT2046;
using libesp::GUI;
using libesp::DisplayMessageState;
using libesp::BaseMenu;
using libesp::System;

const char *DN8App::LOGTAG = "AppTask";
const char *DN8App::sYES = "Yes";
const char *DN8App::sNO = "No";


#define START_ROT libesp::DisplayILI9341::LANDSCAPE_TOP_LEFT
static const uint16_t PARALLEL_LINES = 10;

libesp::DisplayILI9341 Display(DN8App::DISPLAY_WIDTH,DN8App::DISPLAY_HEIGHT,START_ROT,
	PIN_NUM_DISPLAY_BACKLIGHT, PIN_NUM_DISPLAY_RESET);

//static uint16_t BkBuffer[DN8App::FRAME_BUFFER_WIDTH*DN8App::FRAME_BUFFER_HEIGHT];
static uint16_t *BackBuffer = new uint16_t[DN8App::FRAME_BUFFER_WIDTH*DN8App::FRAME_BUFFER_HEIGHT];
//static uint16_t *BackBuffer = &BkBuffer[0];

uint16_t ParallelLinesBuffer[DN8App::DISPLAY_WIDTH*PARALLEL_LINES] = {0};

libesp::ScalingBuffer FrameBuf(&Display, DN8App::FRAME_BUFFER_WIDTH, DN8App::FRAME_BUFFER_HEIGHT, uint8_t(16), DN8App::DISPLAY_WIDTH,DN8App::DISPLAY_HEIGHT, PARALLEL_LINES, (uint8_t*)&BackBuffer[0],(uint8_t*)&ParallelLinesBuffer[0]);

static GUI DN8Gui(&Display);
static ContactStore MyContactStore;
static XPT2046 TouchTask(PIN_NUM_TOUCH_IRQ,true);
static GameTask GMTask("GameTask");
static ButtonInfo MyButtons;
static CalibrationMenu DN8CalibrationMenu;
static TopBoardMenu MyTopBoardMenu;
BluetoothTask *BTTask = NULL; //("BluetoothTask");
WIFITask *WifiTask = NULL; //("WifiTask");

const char *DN8ErrorMap::toString(int32_t err) {
	return "TODO";
}

DN8App DN8App::mSelf;

DN8App &DN8App::get() {
	return mSelf;
}

DN8App::DN8App() : AppErrors() {
	ErrorType::setAppDetail(&AppErrors);
}

DN8App::~DN8App() {

}

XPT2046 &DN8App::getTouch() {
	return TouchTask;
}

ButtonInfo &DN8App::getButtonInfo() {
	return MyButtons;
}

libesp::ErrorType DN8App::onInit() {
	ErrorType et;
	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	MyContactStore.init();
	bool useBT = MyContactStore.getSettings().isBLE();
	if(useBT) {
		BTTask = new BluetoothTask("BlueToothTask");
		ESP_LOGI(LOGTAG,"*************BLE*****************");
	} else {
		ESP_LOGI(LOGTAG,"*************WIFI*****************");
		WifiTask = new WIFITask("WifiTask");
	}
	//ESP_LOGI(LOGTAG,"After Contact Store: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	initialize_pairing_uart();

	et = DN8CalibrationMenu.initNVS();
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to init nvs");
		return et;
	}

	et = XPT2046::initTouch(PIN_NUM_TOUCH_MISO, PIN_NUM_TOUCH_MOSI, PIN_NUM_TOUCH_CLK,VSPI_HOST, 1);
	//ESP_LOGI(LOGTAG,"After Touch and Calibration: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to touch");
		return et;
	}

	libesp::SPIBus* bus = libesp::SPIBus::get(VSPI_HOST);
	et = TouchTask.init(bus,PIN_NUM_TOUCH_CS);

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to touch SPI");
		return et;
	}

	DisplayILI9341::initDisplay(PIN_NUM_DISPLAY_MISO, PIN_NUM_DISPLAY_MOSI,
			PIN_NUM_DISPLAY_CLK, 2, PIN_NUM_DISPLAY_DATA_CMD, PIN_NUM_DISPLAY_RESET,
			PIN_NUM_DISPLAY_BACKLIGHT, HSPI_HOST);

	//ESP_LOGI(LOGTAG,"After Display: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	bus = libesp::SPIBus::get(HSPI_HOST);

	FrameBuf.createInitDevice(bus,PIN_NUM_DISPLAY_CS,PIN_NUM_DISPLAY_DATA_CMD);
	
	ESP_LOGI(LOGTAG,"After FrameBuf: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	ESP_LOGI(LOGTAG,"start display init");
	et=Display.init(libesp::DisplayILI9341::FORMAT_16_BIT, &Font_6x10, &FrameBuf);
	if(et.ok()) {
		ESP_LOGI(LOGTAG,"display init OK");
		Display.fillRec(0,0,FRAME_BUFFER_WIDTH,10,libesp::RGBColor::RED);
		Display.swap();
		Display.fillRec(0,20,FRAME_BUFFER_WIDTH,10,libesp::RGBColor::WHITE);
		Display.swap();
		Display.fillRec(0,40,FRAME_BUFFER_WIDTH,10,libesp::RGBColor::BLUE);
		Display.swap();
		Display.fillRec(0,60,FRAME_BUFFER_WIDTH,10,libesp::RGBColor::GREEN);
		Display.drawRec(0,75,100,10, libesp::RGBColor::BLUE);
		Display.drawString(10,100,"HELLO!",libesp::RGBColor::RED);
		Display.drawString(50,110,"GOODBYE!",libesp::RGBColor::WHITE);
		Display.swap();

		ESP_LOGI(LOGTAG,"After Display swap: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

		// libbt.a is like 300kb
		if(BTTask) {
			if(!BTTask->init()) {
				return ErrorType(BT_INIT_FAIL);
			}
			ESP_LOGI(LOGTAG,"After BT: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
		}

		if(!GMTask.init()) {
			return ErrorType(GAME_TASK_INIT_FAIL);
		}
		ESP_LOGI(LOGTAG,"After GameTask: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

		if(WifiTask) {
			if(!WifiTask->init()) {
				return ErrorType(WIFI_TASK_INIT_FAIL);
			}
			ESP_LOGI(LOGTAG,"After Wifi: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
		}
		if(!MyButtons.init()) {
			return ErrorType(BUTTON_INIT_FAIL);
		} else {
			ESP_LOGI(LOGTAG,"Button Init complete");
		}
		ESP_LOGI(LOGTAG,"After Buttons: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

		if(!MyTopBoardMenu.deviceInit()) {
			return ErrorType(TOP_BOARD_INIT_FAIL);
		} else {
			ESP_LOGI(LOGTAG,"TopBoard Init complete");
		}
		ESP_LOGI(LOGTAG,"After Top Board: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	} else {
		ESP_LOGE(LOGTAG,"failed display init");
	}
	
	TouchTask.start();
	//ESP_LOGI(LOGTAG,"After touch Task start: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	if(BTTask) {
		BTTask->start();
	}
	//ESP_LOGI(LOGTAG,"After BT Task start: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	GMTask.start();
	//ESP_LOGI(LOGTAG,"After GM Task start: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	if(BTTask) {
		BTTask->setGameTaskQueue(GMTask.getQueueHandle());
	}
	if(WifiTask) {
		WifiTask->start();
	}
	ESP_LOGI(LOGTAG,"After Task starts: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
	
	setCurrentMenu(getMenuState());
	return et;
}

BluetoothTask *DN8App::getBTTask() {
	return BTTask;
}

GameTask &DN8App::getGameTask() {
	return GMTask;
}
WIFITask *DN8App::getWifiTask() {
	return WifiTask;
}

uint16_t DN8App::getCanvasWidth() {
	return FrameBuf.getBufferWidth(); 
}

uint16_t DN8App::getCanvasHeight() {
	return FrameBuf.getBufferHeight();
}
uint16_t DN8App::getLastCanvasWidthPixel() {
	return getCanvasWidth()-1;
}

uint16_t DN8App::getLastCanvasHeightPixel() {
	return getCanvasHeight()-1;
}
	
libesp::DisplayDevice &DN8App::getDisplay() {
	return Display;
}

libesp::GUI &DN8App::getGUI() {
	return DN8Gui;
}

ErrorType DN8App::onRun() {
#if 0 
		  return ErrorType();
#else
	MyButtons.process();
	TouchTask.broadcast();
	libesp::BaseMenu::ReturnStateContext rsc = getCurrentMenu()->run();
	Display.swap();

	if (rsc.Err.ok()) {
		if (getCurrentMenu() != rsc.NextMenuToRun) {
			setCurrentMenu(rsc.NextMenuToRun);
			ESP_LOGI(LOGTAG,"on Menu swap: Free: %u, Min %u",
				System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
		} else {
		}
	} else {
		//setCurrentState(StateCollection::getDisplayMessageState(
		//		StateCollection::getDisplayMenuState(), (const char *)"Run State Error....", uint16_t (2000)));
	}
	return ErrorType();
#endif
}

ContactStore &DN8App::getContacts() {
	return MyContactStore;
}

MenuState DN8MenuState;
libesp::DisplayMessageState DMS;
CommunicationSettingState MyCommunicationSettingState;
BadgeInfoMenu MyBadgeInfoMenu;
PairingMenu MyPairingMenu;
GameOfLife MyGameOfLife;
Scan MyWifiScan;
SettingMenu MySettingMenu;
TestMenu MyTestMenu;
Menu3D My3DMenu;
	
BadgeInfoMenu *DN8App::getBadgeInfoMenu() {
	return &MyBadgeInfoMenu;
}

Menu3D *DN8App::get3DMenu() {
	return &My3DMenu;
}

TopBoardMenu *DN8App::getTopBoardMenu() {
	return &MyTopBoardMenu;
}

PairingMenu *DN8App::getPairingMenu() {
	return &MyPairingMenu;
}

GameOfLife *DN8App::getGameOfLifeMenu() {
	return &MyGameOfLife;
}

Scan *DN8App::getWifiScanMenu() {
	return &MyWifiScan;
}

SettingMenu *DN8App::getSettingsMenu() {
	return &MySettingMenu;
}

TestMenu *DN8App::getTestMenu() {
	return &MyTestMenu;
}


CommunicationSettingState *DN8App::getCommunicationSettingState() {
	return &MyCommunicationSettingState;
}


MenuState *DN8App::getMenuState() {
	return &DN8MenuState;
}

CalibrationMenu *DN8App::getCalibrationMenu() {
	return &DN8CalibrationMenu;
}

DisplayMessageState *DN8App::getDisplayMessageState(BaseMenu *bm, const char *msg, uint32_t msDisplay) {
	DMS.setMessage(msg);
	DMS.setNextState(bm);
	DMS.setTimeInState(msDisplay);
	DMS.setDisplay(&Display);
	return &DMS;

}




