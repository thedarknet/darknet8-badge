#ifndef DN8_MENU_STATE_H
#define DN8_MENU_STATE_H

#include "dn8base_menu.h"
#include <libesp/device/display/gui.h>
#include <libesp/device/touch/XPT2046.h>

class MenuState: public DN8BaseMenu {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
	MenuState();
	virtual ~MenuState();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData MenuList;
	libesp::GUIListItemData Items[9];
	QueueHandle_t InternalQueueHandler;
};

#endif
