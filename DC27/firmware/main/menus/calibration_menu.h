#ifndef DN8_CALIBRATION_MENU_H
#define DN8_CALIBRATION_MENU_H

#include "dn8base_menu.h"
#include <libesp/math/point.h>

class CalibrationMenu : public DN8BaseMenu {
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
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::Point2Dus CalibrationLocations[TOTAL];
};

#endif
