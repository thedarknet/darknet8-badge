/*
 * badge_info_state.h
 *
 *      Author: libesp
 */

#ifndef DARKNET_MENUS_BADGE_INFO_MENU_H_
#define DARKNET_MENUS_BADGE_INFO_MENU_H_

#include "dn8base_menu.h"

namespace libesp {
	class TouchNotification;
}

class ContactStore;

class BadgeInfoMenu: public DN8BaseMenu {
public:
	static const int TOUCH_QUEUE_SIZE = 10;
	static const int TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
	BadgeInfoMenu();
	virtual ~BadgeInfoMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
	const char *getRegCode();
private:
	libesp::GUIListData BadgeInfoList;
	libesp::GUIListItemData Items[9];
	char RegCode[19];
	QueueHandle_t TouchQueueHandle;
	static const uint16_t ItemCount = (sizeof(Items)/sizeof(Items[0]));
};


#endif /* DARKNET_MENUS_BADGE_INFO_STATE_H_ */
