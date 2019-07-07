#ifndef DN8_MENU_STATE_H
#define DN8_MENU_STATE_H

#include "dn8base_menu.h"
#include <libesp/device/display/gui.h>

class MenuState: public DN8BaseMenu {
public:
	MenuState();
	virtual ~MenuState();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData MenuList;
	libesp::GUIListItemData Items[12];
};

#endif
