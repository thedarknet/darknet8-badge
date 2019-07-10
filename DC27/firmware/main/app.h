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
};

enum ERRORS {
	APP_OK = 0
	, OTA_INIT_FAIL = 1
	, BT_INIT_FAIL
	, GAME_TASK_INIT_FAIL
	, EXPLOIT_TASK_INIT_FAIL
};

class DN8ErrorMap : public libesp::IErrorDetail {
public:
	virtual const char *toString(uint32_t err);
};

class BluetoothTask;
class GameTask;
class ExploitableGameTask;
class OTATask;

class DN8App : public libesp::App {
public:
	struct DN8AppMsg {
		int dummy;
	};
public:
	static const char *LOGTAG;
	static const int QUEUE_SIZE = 10;
	static const int ITEM_SIZE = sizeof(DN8AppMsg);
	static DN8App &get();
public:
	virtual ~DN8App();
	BluetoothTask &getBTTask();
	GameTask &getGameTask();
	ExploitableGameTask &getExploitTask();
	OTATask &getOTATask();
	uint16_t getCanvasWidth();
	uint16_t getCanvasHeight();
	libesp::DisplayDevice &getDisplay();
	libesp::GUI &getGUI();
protected:
	DN8App();
	virtual libesp::ErrorType onInit();
	virtual libesp::ErrorType onRun();
private:
	static DN8App mSelf;
	DN8ErrorMap AppErrors;
};

#endif /* DC27_APP_H */
