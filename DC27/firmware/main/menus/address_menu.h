#ifndef ADDRESS_MENU_H
#define ADDRESS_MENU_H

#include "dn8base_menu.h"
#include "../KeyStore.h"

class AddressMenu: public DN8BaseMenu {
public:
	AddressMenu();
	virtual ~AddressMenu();
	void resetSelection();
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::StateBase::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
	void setNext4Items(uint16_t startAt);
private:
	libesp::GUIListData AddressList;
	libesp::GUIListItemData Items[4];
	ContactStore::Contact CurrentContactList[4];
	libesp::GUIListData ContactDetails;
	libesp::GUIListItemData DetailItems[5];
	char RadioIDBuf[12];
	char PublicKey[64];
	char SignatureKey[128];
	uint8_t Index;
	libesp::GUIListData *DisplayList;
};


#endif
