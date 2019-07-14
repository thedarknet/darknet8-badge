#ifndef DN8_BASE_MENU_H
#define DN8_BASE_MENU_H

#include <libesp/app/basemenu.h>
#include <libesp/device/display/gui.h>

class DN8BaseMenu : public libesp::BaseMenu {
public:
	DN8BaseMenu() : libesp::BaseMenu() {}
	virtual ~DN8BaseMenu(){}
};

#endif
