/*
 * app.cpp
 *
 * Author: cmdc0de
 */

#include "app.h"
#include <esp_log.h>
#include <libesp/system.h>
#include <libesp/spibus.h>
#include <libesp/device/display/frame_buffer.h>
#include <libesp/device/display/display_device.h>
#include <libesp/device/display/fonts.h>
#include <libesp/device/display/gui.h>
#include <libesp/device/touch/XPT2046.h>
#include "./ble.h"
#include "./game_master.h"
#include "./ota.h"
// XXX: Games get included here and installed into the game master
#include "./exploitable.h"
#include "./brainfuzz.h"
#include "./ttt3d.h"
#include "menus/menu_state.h"
#include "buttons.h"
#include "menus/calibration_menu.h"

using libesp::ErrorType;
using libesp::DisplayILI9341;
using libesp::XPT2046;
using libesp::GUI;

const char *DN8App::LOGTAG = "AppTask";
static StaticQueue_t InCommingQueue;
static uint8_t CommandBuffer[DN8App::QUEUE_SIZE * DN8App::ITEM_SIZE] = { 0 };

#define PIN_NUM_TOUCH_MISO GPIO_NUM_35
#define PIN_NUM_TOUCH_MOSI GPIO_NUM_33
#define PIN_NUM_TOUCH_CLK  GPIO_NUM_26
#define PIN_NUM_TOUCH_CS   GPIO_NUM_27
#define PIN_NUM_TOUCH_IRQ  GPIO_NUM_32

#define PIN_NUM_DISPLAY_MISO GPIO_NUM_19
#define PIN_NUM_DISPLAY_MOSI GPIO_NUM_23
#define PIN_NUM_DISPLAY_CLK  GPIO_NUM_18
#define PIN_NUM_DISPLAY_CS  GPIO_NUM_17
#define PIN_NUM_DISPLAY_DATA_CMD GPIO_NUM_4
#define PIN_NUM_DISPLAY_BACKLIGHT NOPIN
#define PIN_NUM_DISPLAY_RESET NOPIN

static const uint16_t DISPLAY_HEIGHT		= 240;
static const uint16_t DISPLAY_WIDTH			= 320;
//static const uint16_t FRAME_BUFFER_HEIGHT	= 120;
//static const uint16_t FRAME_BUFFER_WIDTH	= 160;
static const uint16_t FRAME_BUFFER_HEIGHT	= 156;
static const uint16_t FRAME_BUFFER_WIDTH	= 208;
#define START_ROT libesp::DisplayILI9341::LANDSCAPE_TOP_LEFT
static const uint16_t PARALLEL_LINES = 10;

libesp::DisplayILI9341 Display(DISPLAY_WIDTH,DISPLAY_HEIGHT,START_ROT, NOPIN, NOPIN);

uint16_t *BackBuffer = new uint16_t[FRAME_BUFFER_WIDTH*FRAME_BUFFER_HEIGHT];
uint16_t ParallelLinesBuffer[DISPLAY_WIDTH*PARALLEL_LINES] = {0};

libesp::ScalingBuffer FrameBuf(&Display, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, uint8_t(16), DISPLAY_WIDTH,DISPLAY_HEIGHT, PARALLEL_LINES, (uint8_t*)&BackBuffer[0],(uint8_t*)&ParallelLinesBuffer[0]);

GUI DN8Gui(&Display);
XPT2046 TouchTask(4,25,PIN_NUM_TOUCH_IRQ);
BluetoothTask BTTask("BluetoothTask");
GameTask GMTask("GameTask");
ExploitableGameTask ExploitTask("ExploitTask");
BrainfuzzGameTask BrainfuzzTask("BrainfuzzTask");
TTT3DGameTask TTT3DTask("TTT3DTask");
OTATask OtaTask("OTATask");
ButtonInfo MyButtons;
MenuState DN8MenuState;
CalibrationMenu DN8CalibrationMenu;

const char *DN8ErrorMap::toString(uint32_t err) {
	return "TODO";
}

MenuState *DN8App::getMenuState() {
	return &DN8MenuState;
}

CalibrationMenu *DN8App::getCalibrationMenu() {
	return &DN8CalibrationMenu;
}

DN8App DN8App::mSelf;

DN8App &DN8App::get() {
	return mSelf;
}

DN8App::DN8App() : AppErrors() {
	ErrorType::setFacilityErrorDetailHandler(ErrorType::FACILITY_APP,&AppErrors);
}

DN8App::~DN8App() {

}

ButtonInfo &DN8App::getButtonInfo() {
	return MyButtons;
}

libesp::ErrorType DN8App::onInit() {
	ErrorType et;
	
	et = XPT2046::initTouch(PIN_NUM_TOUCH_MISO, PIN_NUM_TOUCH_MOSI, PIN_NUM_TOUCH_CLK,VSPI_HOST, 1);

	if(!et.ok()) {
		return et;
	}

	libesp::SPIBus* bus = libesp::SPIBus::get(VSPI_HOST);
	et = TouchTask.init(bus,PIN_NUM_TOUCH_CS);

	if(!et.ok()) {
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
		Display.fillRec(0,0,FRAME_BUFFER_WIDTH,20,libesp::RGBColor::RED);
		Display.swap();
		Display.fillRec(0,30,FRAME_BUFFER_WIDTH,20,libesp::RGBColor::WHITE);
		Display.swap();
		Display.fillRec(0,60,FRAME_BUFFER_WIDTH,20,libesp::RGBColor::BLUE);
		Display.swap();
		Display.fillRec(0,90,FRAME_BUFFER_WIDTH,20,libesp::RGBColor::GREEN);
		Display.drawRec(0,115,100,10, libesp::RGBColor::BLUE);
		Display.drawString(10,130,"HELLO!",libesp::RGBColor::RED);
		Display.drawString(50,140,"GOODBYE!",libesp::RGBColor::WHITE);
		Display.swap();
		ESP_LOGI(LOGTAG,"display init swap done");


		// XXX: In the final version, start this task first, because it monitors
		// whether a boot was successful and will provider an interposition point
		// for a user to select whether to revert to the last known bootable
		// partition if the app has crashed x times.  (A crash defined as not
		// living beyond 30 seconds - easy revert by resetting the badge x times).
		// libwifi and friends is like 500kb
		if(!OtaTask.init()) {
			return ErrorType(ErrorType::FACILITY_APP,OTA_INIT_FAIL);
		}
		//OTACmd* cmd = (OTACmd*)malloc(sizeof(OTACmd));
		// *cmd = ATTEMPT_OTA;
		//xQueueSend(OTATask.getQueueHandle(), &cmd, (TickType_t)100);
	
		////
		// libbt.a is like 300kb
		if(!BTTask.init()) {
			return ErrorType(ErrorType::FACILITY_APP,BT_INIT_FAIL);
		}

		if(!GMTask.init()) {
			return ErrorType(ErrorType::FACILITY_APP,GAME_TASK_INIT_FAIL);
		}

		if(!ExploitTask.init()) {
			return ErrorType(ErrorType::FACILITY_APP,EXPLOIT_TASK_INIT_FAIL);
		}
		if(!BrainfuzzTask.init()) {
			return ErrorType(ErrorType::FACILITY_APP,EXPLOIT_TASK_INIT_FAIL);
		}
		if(!TTT3DTask.init()) {
			return ErrorType(ErrorType::FACILITY_APP,EXPLOIT_TASK_INIT_FAIL);
		}
		if(!MyButtons.init()) {
			return ErrorType(ErrorType::FACILITY_APP,BUTTON_INIT_FAIL);
		} else {
			ESP_LOGI(LOGTAG,"Button Init complete");
		}
	} else {
		ESP_LOGE(LOGTAG,"failed display init");
	}
	
	TouchTask.start();
	OtaTask.start();
	BTTask.start();
	GMTask.start();
	BTTask.setGameTaskQueue(GMTask.getQueueHandle());
	ExploitTask.start();
	BrainfuzzTask.start();
	TTT3DTask.start();
	GMTask.installGame(EXPLOITABLE_ID, true, ExploitTask.getQueueHandle());
	GMTask.installGame(BRAINFUZZ_ID, true, BrainfuzzTask.getQueueHandle());
	GMTask.installGame(TTT3D_ID, true, TTT3DTask.getQueueHandle());
	
	setCurrentMenu(getMenuState());
	return et;
}

BluetoothTask &DN8App::getBTTask() {
	return BTTask;
}

GameTask &DN8App::getGameTask() {
	return GMTask;
}

ExploitableGameTask &DN8App::getExploitTask() {
	return ExploitTask;
}

BrainfuzzGameTask &DN8App::getBrainfuzzTask() {
	return BrainfuzzTask;
}

TTT3DGameTask &DN8App::getTTT3DTask() {
	return TTT3DTask;
}

OTATask &DN8App::getOTATask() {
	return OtaTask;
}

uint16_t DN8App::getCanvasWidth() {
	return FrameBuf.getBufferWidth(); 
}

uint16_t DN8App::getCanvasHeight() {
	return FrameBuf.getBufferHeight();
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

