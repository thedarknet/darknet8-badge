/*
 * setting_state.h
 *
 *      Author: libesp
 */

#ifndef DARKNET_MENUS_SETTING_STATE_H_
#define DARKNET_MENUS_SETTING_STATE_H_

#include "dn8base_menu.h"
#include "../KeyStore.h"
#include "virtual_key_board.h"

namespace libesp {
	class TouchNotification;
}

class SettingMenu: public DN8BaseMenu {
public:
	SettingMenu();
	virtual ~SettingMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData SettingList;
	libesp::GUIListItemData Items[10];
	char AgentName[Contact::AGENT_NAME_LENGTH];
	uint8_t SubState;
	uint8_t MiscCounter;
	VirtualKeyBoard VKB;
	VirtualKeyBoard::InputHandleContext IHC;
	QueueHandle_t TouchQueueHandle;
public:
	static const int TOUCH_QUEUE_SIZE = 4;
	static const int TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
	static const uint16_t ItemCount = (sizeof(Items)/sizeof(Items[0]));
	static const char *LOGTAG;
};


#endif /* DARKNET_MENUS_SETTING_STATE_H_ */
