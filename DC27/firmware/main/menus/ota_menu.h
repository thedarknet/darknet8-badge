/*
 * ota_state.h
 *
 *      Author: libesp
 */

#ifndef DARKNET_MENUS_OTA_MENU_H_
#define DARKNET_MENUS_OTA_MENU_H_

#include "dn8base_menu.h"

class ContactStore;

class OTAMenu: public DN8BaseMenu {
public:
	OTAMenu();
	virtual ~OTAMenu();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
private:
	libesp::GUIListData OTAList;
	libesp::GUIListItemData Items[1];
	uint32_t my_nvs_handle;
	void do_factory_reset(void);
};


#endif /* DARKNET_MENUS_OTA_STATE_H_ */
