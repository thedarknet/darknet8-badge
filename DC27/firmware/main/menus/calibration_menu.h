#ifndef DN8_CALIBRATION_MENU_H
#define DN8_CALIBRATION_MENU_H

#include "dn8base_menu.h"
#include <libesp/math/point.h>
#include <libesp/device/touch/XPT2046.h>
#include <libesp/nvs_memory.h>

class CalibrationMenu : public DN8BaseMenu {
public:
	static const int QUEUE_SIZE = 2;
	static const int MSG_SIZE = sizeof(libesp::XPT2046::TouchNotification*);
	static const char *LOGTAG;
public:
	enum CALIBRATION_LOCATION {
		MID = 0
		, TOP_LEFT
		, TOP_RIGHT
		, BOTTOM_LEFT
		, BOTTOM_RIGHT
		, LAST = BOTTOM_RIGHT
		, TOTAL = LAST + 1
	};
public:
	CalibrationMenu();
	virtual ~CalibrationMenu();
	libesp::ErrorType initNVS();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
	void drawCrossHairs();
	libesp::ErrorType loadCalibrationData();
private:
	libesp::Point2Ds CalibrationLocations[TOTAL];
	uint32_t CurrentIndex;
	QueueHandle_t InternalQueueHandler;
	libesp::NVS CalibrationData;
};

#endif
