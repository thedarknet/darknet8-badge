#ifndef TOP_BOARD_MENU_H
#define TOP_BOARD_MENU_H

#include "dn8base_menu.h"

namespace libesp {
	class TouchNotification;
}

class TopBoardMenu: public DN8BaseMenu {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
public:
	TopBoardMenu();
	bool deviceInit(); 
	virtual ~TopBoardMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData DeviceList;
	libesp::GUIListItemData Items[9];
	QueueHandle_t InternalQueueHandler;
};
#endif
