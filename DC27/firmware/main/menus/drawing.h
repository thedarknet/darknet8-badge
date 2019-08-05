#ifndef DRAWING_MENU_H
#define DRAWING_MENU_H

#include "dn8base_menu.h"

namespace libesp {
	class TouchNotification;
}

class DrawingMenu: public DN8BaseMenu {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
public:
	DrawingMenu();
	virtual ~DrawingMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
protected:
	void drawVirtualButtons();
	libesp::BaseMenu* processVirtualButtons();
	libesp::BaseMenu*  processButtons();
private:
	QueueHandle_t InternalQueueHandler;
	uint8_t ColorIndex;	
};
#endif
