#ifndef DRAWING_MENU_H
#define DRAWING_MENU_H

#include "dn8base_menu.h"
#include <math/point.h>

namespace libesp {
	class TouchNotification;
}

class DrawingMenu: public DN8BaseMenu {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
	static const uint8_t MAX_SAVE_SLOTS = 3;
public:
	DrawingMenu();
	virtual ~DrawingMenu();
	libesp::ErrorType initStorage();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
protected:
	void drawVirtualButtons();
	libesp::BaseMenu* processVirtualButtons(libesp::Point2Ds &touchPoint);
	libesp::BaseMenu*  processButtons();
private:
	QueueHandle_t InternalQueueHandler;
	uint8_t ColorIndex;	
	int8_t LastSaveIndex;
};
#endif
