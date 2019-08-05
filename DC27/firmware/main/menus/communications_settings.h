/*
 * communications_settings.h
 *
 *      Author: libesp
 */

#ifndef DARKNET_MENUS_COMMUNICATIONS_SETTINGS_H_
#define DARKNET_MENUS_COMMUNICATIONS_SETTINGS_H_

#include "dn8base_menu.h"
#include "virtual_key_board.h"

namespace libesp {
	class TouchNotification;
}

class WIFIResponseMsg;

class CommunicationSettingState: public DN8BaseMenu {
public:
	static const int TOUCH_QUEUE_SIZE = 8;
	static const int TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
	static const int WIFI_QUEUE_SIZE = 4;
	static const int WIFI_MSG_SIZE = sizeof(WIFIResponseMsg*);
	CommunicationSettingState();
	virtual ~CommunicationSettingState();
protected:
	enum INTERNAL_STATE { NONE, FETCHING_DATA, DISPLAY_DATA };
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData CommSettingList;
	libesp::GUIListItemData Items[5];
	char CurrentDeviceName[13];
	INTERNAL_STATE InternalState;
	QueueHandle_t WifiQueueHandle;
	QueueHandle_t TouchQueueHandle;
public:
	static const uint16_t ItemCount = uint16_t(sizeof(Items)/sizeof(Items[0]));
};



#endif /* DARKNET_MENUS_COMMUNICATIONS_SETTINGS_H_ */
