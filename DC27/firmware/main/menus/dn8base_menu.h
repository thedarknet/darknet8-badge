#ifndef DN8_BASE_MENU_H
#define DN8_BASE_MENU_H

#include <app/basemenu.h>
#include <device/display/gui.h>

class DN8BaseMenu : public libesp::BaseMenu {
public:
	DN8BaseMenu() : libesp::BaseMenu() {}
	virtual ~DN8BaseMenu(){}
protected:
	static void clearListBuffer();
	static char *getRow(uint8_t row);
private:
	static char ListBuffer[10][32]; //height then width
	static uint8_t NumRows;
};

#endif
