#ifndef DN8_BASE_MENU_H
#define DN8_BASE_MENU_H

#include <app/basemenu.h>
#include <device/display/gui.h>

namespace libesp {
class TouchNotification;
class GUIListData;
}

class DN8BaseMenu : public libesp::BaseMenu {
public:
	static const char *LOGTAG;
public:
	DN8BaseMenu() : libesp::BaseMenu() {}
	virtual ~DN8BaseMenu(){}
protected:
	static void clearListBuffer();
	static char *getRow(uint8_t row);
protected:
	libesp::TouchNotification *processTouch(QueueHandle_t &queH, libesp::GUIListData &guiList, uint16_t itemCount, bool &penUp, bool &headerHit);
	bool backAction();
	bool selectAction();
	bool upAction();
	bool downAction();
private:
	static char ListBuffer[10][32]; //height then width
	static uint8_t NumRows;
};

#endif
