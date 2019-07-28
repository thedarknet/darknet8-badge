/*
 * app.h
 *
 * Author: cmdc0de
 */

#ifndef DC27_APP_H
#define DC27_APP_H

#include <app/app.h>

namespace libesp {
class GUI;
class DisplayDevice;
class DisplayMessageState;
class XPT2046;
};

enum ERRORS {
	APP_OK = libesp::ErrorType::APP_OK
	, OTA_INIT_FAIL = libesp::ErrorType::APP_BASE + 1
	, BT_INIT_FAIL
	, GAME_TASK_INIT_FAIL
	, EXPLOIT_TASK_INIT_FAIL
	, WIFI_TASK_INIT_FAIL
	, BUTTON_INIT_FAIL
};

class DN8ErrorMap : public libesp::IErrorDetail {
public:
	virtual const char *toString(int32_t err);
};

class BluetoothTask;
class GameTask;
class WIFITask;
class ButtonInfo;
class MenuState;
class CalibrationMenu;
class ContactStore;
class CommunicationSettingState;
class BadgeInfoMenu;
class OTAMenu;
class PairingMenu;
class GameOfLife;
class Scan;
class SettingMenu;
class TestMenu;

class DN8App : public libesp::App {
public:
	struct DN8AppMsg {
		int dummy;
	};
public:
	static const char *LOGTAG;
	static const int QUEUE_SIZE = 10;
	static const int ITEM_SIZE = sizeof(DN8AppMsg);
	static const char *sYES;
	static const char *sNO;
	static const uint16_t DISPLAY_HEIGHT		= 240;
	static const uint16_t DISPLAY_WIDTH			= 320;
	static const uint16_t FRAME_BUFFER_HEIGHT	= 144;
	static const uint16_t FRAME_BUFFER_WIDTH	= 192;
	static DN8App &get();
public:
	virtual ~DN8App();
	BluetoothTask &getBTTask();
	GameTask &getGameTask();
	WIFITask &getWifiTask();
	uint16_t getCanvasWidth();
	uint16_t getCanvasHeight();
	uint16_t getLastCanvasWidthPixel();
	uint16_t getLastCanvasHeightPixel();
	libesp::DisplayDevice &getDisplay();
	libesp::GUI &getGUI();
	ButtonInfo &getButtonInfo();
	MenuState *getMenuState();
	CalibrationMenu *getCalibrationMenu();
	ContactStore &getContacts();
	libesp::DisplayMessageState *getDisplayMessageState(libesp::BaseMenu *, const char *msg, uint32_t msDisplay);
	CommunicationSettingState *getCommunicationSettingState();
	BadgeInfoMenu *getBadgeInfoMenu();
	OTAMenu *getOTAMenu();
	PairingMenu *getPairingMenu();
	GameOfLife *getGameOfLifeMenu();
	Scan *getWifiScanMenu();
	SettingMenu *getSettingsMenu();
	TestMenu *getTestMenu();
	libesp::XPT2046 &getTouch();
protected:
	DN8App();
	virtual libesp::ErrorType onInit();
	virtual libesp::ErrorType onRun();
private:
	static DN8App mSelf;
	DN8ErrorMap AppErrors;
};

#endif /* DC27_APP_H */
