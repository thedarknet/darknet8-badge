/*
 *
 *      Author: libesp
 */

#ifndef DARKNET_MENUS_SCAN_H_
#define DARKNET_MENUS_SCAN_H_

#include "dn8base_menu.h"


class Scan: public DN8BaseMenu {
public:
	Scan();
	virtual ~Scan();
	void setNPCOnly(bool b) {NPCOnly = b;}
	bool isNPCOnly() {return NPCOnly;}
	enum INTERNAL_STATE {NONE, FETCHING_DATA, DISPLAY_DATA};
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	bool NPCOnly;
	libesp::GUIListData DisplayList;
	libesp::GUIListItemData Items[5];
	char ListBuffer[5][48]; //height then width
	INTERNAL_STATE InternalState;
};




#endif /* DARKNET_MENUS_TAMAGOTCHI_H_ */
