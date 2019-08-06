#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include "dn8base_menu.h"

namespace libesp {
	class TouchNotification;
}

class TestMenu: public DN8BaseMenu {
public:
	static const int QUEUE_SIZE = 10;
	static const int MSG_SIZE = sizeof(libesp::TouchNotification*);
public:
	TestMenu();
	virtual ~TestMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData ButtonList;
	libesp::GUIListItemData Items[9];
	int32_t PenX;
	int32_t PenY;
	int32_t PenZ;
	bool IsPenDown;
	QueueHandle_t InternalQueueHandler;
public:
	static const uint16_t ItemCount = (sizeof(Items)/sizeof(Items[0]));
};
#endif
