/*
 *
 *      Author: libesp
 */

#ifndef DARKNET_MENUS_SCAN_H_
#define DARKNET_MENUS_SCAN_H_

#include "dn8base_menu.h"

namespace libesp {
	class TouchNotification;
}

class WIFIResponseMsg;

class Scan: public DN8BaseMenu {
public:
	Scan();
	virtual ~Scan();
	void setNPCOnly(bool b) {NPCOnly = b;}
	bool isNPCOnly() {return NPCOnly;}
	enum INTERNAL_STATE {NONE, FETCHING_DATA, DISPLAY_DATA};
	static const int WIFI_QUEUE_SIZE = 4;
	static const int WIFI_MSG_SIZE = sizeof(WIFIResponseMsg*);
	static const int TOUCH_QUEUE_SIZE = 8;
	static const int TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	bool NPCOnly;
	libesp::GUIListData DisplayList;
	libesp::GUIListItemData Items[9];
	INTERNAL_STATE InternalState;
	static const uint16_t ItemCount = (sizeof(Items)/sizeof(Items[0]));
	QueueHandle_t WifiQueueHandle;
	QueueHandle_t TouchQueueHandle;
	WIFIResponseMsg *ResponseMsg;
};




#endif /* */
