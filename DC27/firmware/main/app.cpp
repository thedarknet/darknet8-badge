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
//#include "./wifi.h"
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

using libesp::ErrorType;
using libesp::DisplayILI9341;
using libesp::XPT2046;
using libesp::GUI;
using libesp::DisplayMessageState;
using libesp::BaseMenu;

const char *DN8App::LOGTAG = "AppTask";
static StaticQueue_t InCommingQueue;
static uint8_t CommandBuffer[DN8App::QUEUE_SIZE * DN8App::ITEM_SIZE] = { 0 };
const char *DN8App::sYES = "Yes";
const char *DN8App::sNO = "No";


#define PIN_NUM_TOUCH_CLK  GPIO_NUM_26
#define PIN_NUM_TOUCH_CS   GPIO_NUM_27
#define PIN_NUM_TOUCH_IRQ  GPIO_NUM_32
#define PIN_NUM_TOUCH_MISO GPIO_NUM_35
#define PIN_NUM_TOUCH_MOSI GPIO_NUM_33

//#define DEVKIT_LCD

#ifdef DEVKIT_LCD
#define PIN_NUM_DISPLAY_RESET GPIO_NUM_18
#define PIN_NUM_DISPLAY_CLK  GPIO_NUM_19
#define PIN_NUM_DISPLAY_DATA_CMD GPIO_NUM_21
#define PIN_NUM_DISPLAY_CS  GPIO_NUM_22
#define PIN_NUM_DISPLAY_MISO GPIO_NUM_23
#define PIN_NUM_DISPLAY_MOSI GPIO_NUM_25
#define PIN_NUM_DISPLAY_BACKLIGHT GPIO_NUM_5
#else
#define PIN_NUM_DISPLAY_MISO GPIO_NUM_19
#define PIN_NUM_DISPLAY_MOSI GPIO_NUM_23
#define PIN_NUM_DISPLAY_CLK  GPIO_NUM_18
#define PIN_NUM_DISPLAY_CS  GPIO_NUM_17
#define PIN_NUM_DISPLAY_DATA_CMD GPIO_NUM_4
#define PIN_NUM_DISPLAY_BACKLIGHT NOPIN
#define PIN_NUM_DISPLAY_RESET NOPIN
#endif

//static const uint16_t FRAME_BUFFER_HEIGHT	= 132;
//static const uint16_t FRAME_BUFFER_WIDTH	= 176;
#define START_ROT libesp::DisplayILI9341::LANDSCAPE_TOP_LEFT
static const uint16_t PARALLEL_LINES = 10;

libesp::DisplayILI9341 Display(DN8App::DISPLAY_WIDTH,DN8App::DISPLAY_HEIGHT,START_ROT, NOPIN, NOPIN);

uint16_t *BackBuffer = new uint16_t[DN8App::FRAME_BUFFER_WIDTH*DN8App::FRAME_BUFFER_HEIGHT];
uint16_t ParallelLinesBuffer[DN8App::DISPLAY_WIDTH*PARALLEL_LINES] = {0};

libesp::ScalingBuffer FrameBuf(&Display, DN8App::FRAME_BUFFER_WIDTH, DN8App::FRAME_BUFFER_HEIGHT, uint8_t(16), DN8App::DISPLAY_WIDTH,DN8App::DISPLAY_HEIGHT, PARALLEL_LINES, (uint8_t*)&BackBuffer[0],(uint8_t*)&ParallelLinesBuffer[0]);

GUI DN8Gui(&Display);
ContactStore MyContactStore;
XPT2046 TouchTask(PIN_NUM_TOUCH_IRQ,true);
BluetoothTask BTTask("BluetoothTask");
GameTask GMTask("GameTask");
ButtonInfo MyButtons;
CalibrationMenu DN8CalibrationMenu;
//WIFITask WifiTask("WifiTask");

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

	MyContactStore.init();

	et = DN8CalibrationMenu.initNVS();
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to init nvs");
		return et;
	}

	et = XPT2046::initTouch(PIN_NUM_TOUCH_MISO, PIN_NUM_TOUCH_MOSI, PIN_NUM_TOUCH_CLK,VSPI_HOST, 1);

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

	bus = libesp::SPIBus::get(HSPI_HOST);

	FrameBuf.createInitDevice(bus,PIN_NUM_DISPLAY_CS,PIN_NUM_DISPLAY_DATA_CMD);

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
		ESP_LOGI(LOGTAG,"display init swap done");


		// libbt.a is like 300kb
		if(!BTTask.init()) {
			return ErrorType(BT_INIT_FAIL);
		}

		if(!GMTask.init()) {
			return ErrorType(GAME_TASK_INIT_FAIL);
		}
/*
		if(!WifiTask.init()) {
			return ErrorType(WIFI_TASK_INIT_FAIL);
		}
*/
		if(!MyButtons.init()) {
			return ErrorType(BUTTON_INIT_FAIL);
		} else {
			ESP_LOGI(LOGTAG,"Button Init complete");
		}
	} else {
		ESP_LOGE(LOGTAG,"failed display init");
	}
	
	TouchTask.start();
	BTTask.start();
	GMTask.start();
	BTTask.setGameTaskQueue(GMTask.getQueueHandle());
	//WifiTask.start();
	
	setCurrentMenu(getMenuState());
	return et;
}

BluetoothTask &DN8App::getBTTask() {
	return BTTask;
}

GameTask &DN8App::getGameTask() {
	return GMTask;
}

/*
WIFITask &DN8App::getWifiTask() {
	return WifiTask;
}
*/

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
	
BadgeInfoMenu *DN8App::getBadgeInfoMenu() {
	return &MyBadgeInfoMenu;
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




