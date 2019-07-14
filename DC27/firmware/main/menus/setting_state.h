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
	libesp::GUIListItemData Items[3];
	char AgentName[ContactStore::AGENT_NAME_LENGTH];
	uint8_t SubState;
	uint8_t MiscCounter;
	VirtualKeyBoard VKB;
	VirtualKeyBoard::InputHandleContext IHC;
};


#endif /* DARKNET_MENUS_SETTING_STATE_H_ */
