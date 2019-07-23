#ifndef DN8_CALIBRATION_MENU_H
#define DN8_CALIBRATION_MENU_H

#include "dn8base_menu.h"
#include <math/point.h>
#include <device/touch/XPT2046.h>
#include <nvs_memory.h>
#include <math/point.h>

class CalibrationMenu : public DN8BaseMenu {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
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
	libesp::Point2Ds getPickPoint(const libesp::Point2Ds &pickPos);
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
	void drawCrossHairs();
	libesp::ErrorType loadCalibrationData();
	void calculate();
private:
	libesp::Point2Ds CalibrationLocations[TOTAL];
	uint32_t CurrentIndex;
	QueueHandle_t InternalQueueHandler;
	libesp::NVS CalibrationData;
	libesp::Point2Ds Min;
	libesp::Point2Ds Max;
	libesp::Point2Ds Range;
};

#endif
