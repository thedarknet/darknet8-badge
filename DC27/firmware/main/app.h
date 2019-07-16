/*
 * app.h
 *
 * Author: cmdc0de
 */

#ifndef DC27_APP_H
#define DC27_APP_H

#include <libesp/app/app.h>

namespace libesp {
class GUI;
class DisplayDevice;
class DisplayMessageState;
};

enum ERRORS {
	APP_OK = 0
	, OTA_INIT_FAIL = 1
	, BT_INIT_FAIL
	, GAME_TASK_INIT_FAIL
	, EXPLOIT_TASK_INIT_FAIL
	, BUTTON_INIT_FAIL
};

class DN8ErrorMap : public libesp::IErrorDetail {
public:
	virtual const char *toString(uint32_t err);
};

class BluetoothTask;
class GameTask;
class ExploitableGameTask;
class BrainfuzzGameTask;
class TTT3DGameTask;
class OTATask;
class ButtonInfo;
class MenuState;
class CalibrationMenu;
class ContactStore;
class CommunicationSettingState;
class BadgeInfoMenu;
class OTAMenu;
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
	ExploitableGameTask &getExploitTask();
	BrainfuzzGameTask &getBrainfuzzTask();
	TTT3DGameTask &getTTT3DTask();
	OTATask &getOTATask();
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
	GameOfLife *getGameOfLifeMenu();
	Scan *getWifiScanMenu();
	SettingMenu *getSettingsMenu();
	TestMenu *getTestMenu();
protected:
	DN8App();
	virtual libesp::ErrorType onInit();
	virtual libesp::ErrorType onRun();
private:
	static DN8App mSelf;
	DN8ErrorMap AppErrors;
};

#endif /* DC27_APP_H */
