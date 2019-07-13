/*
 * communications_settings.h
 *
 *      Author: libesp
 */

#ifndef DARKNET_MENUS_COMMUNICATIONS_SETTINGS_H_
#define DARKNET_MENUS_COMMUNICATIONS_SETTINGS_H_

#include "dn8base_menu.h"
#include "virtual_key_board.h"


class CommunicationSettingState: public DN8BaseMenu {
public:
	CommunicationSettingState();
	virtual ~CommunicationSettingState();
protected:
	enum INTERNAL_STATE { NONE, FETCHING_DATA, DISPLAY_DATA };
	virtual libesp::ErrorType onInit();
	virtual libesp::StateBase::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData CommSettingList;
	libesp::GUIListItemData Items[3];
	char ListBuffer[3][24];
	char CurrentDeviceName[13];
	INTERNAL_STATE InternalState;
};



#endif /* DARKNET_MENUS_COMMUNICATIONS_SETTINGS_H_ */
