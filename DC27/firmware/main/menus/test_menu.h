#ifndef EVENT_STATE_H
#define EVENT_STATE_H

#include "dn8base_menu.h"

class TestMenu: public DN8BaseMenu {
public:
	static const uint16_t MAX_MSG_LEN = 64;
public:

public:
	TestMenu();
	virtual ~TestMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	static const uint32_t EXIT_COUNT = 20;
	libesp::GUIListData ButtonList;
	libesp::GUIListItemData Items[8];
	uint32_t TimesMidHasBeenHeld;
};
#endif
