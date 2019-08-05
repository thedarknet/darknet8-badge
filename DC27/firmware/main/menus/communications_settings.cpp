/*
 * communications_settings.cpp
 *
 *      Author: cmdc0de
 */


#include "communications_settings.h"
#include "gui_list_processor.h"
#include "menu_state.h"
#include "../app.h"
#include "../buttons.h"
#include "../wifi.h"
#include "../ble.h"
#include "../KeyStore.h"
#include <device/touch/XPT2046.h>
#include "app/display_message_state.h"

using libesp::ErrorType;
using libesp::RGBColor;
using libesp::BaseMenu;
using libesp::TouchNotification;

static StaticQueue_t WIFIQueue;
static uint8_t WIFIQueueBuffer[CommunicationSettingState::WIFI_QUEUE_SIZE*CommunicationSettingState::WIFI_MSG_SIZE] = {0};
static StaticQueue_t TouchQueue;
static uint8_t TouchQueueBuffer[CommunicationSettingState::TOUCH_QUEUE_SIZE*CommunicationSettingState::TOUCH_MSG_SIZE] = {0};

class BLESetDeviceNameMenu: public DN8BaseMenu {
private:
	VirtualKeyBoard VKB;
	char NewDeviceName[13];
	VirtualKeyBoard::InputHandleContext IHC;
	const char *CurrentDeviceName;
	//uint32_t RequestID;
public:
	void setCurrentNamePtr(const char *p) {CurrentDeviceName = p;}
	BLESetDeviceNameMenu() : DN8BaseMenu(), VKB(), NewDeviceName(), IHC(&NewDeviceName[0],sizeof(NewDeviceName)),
			CurrentDeviceName(0) { //, RequestID(0) {

	}
	virtual ~BLESetDeviceNameMenu() {}
protected:
	virtual libesp::ErrorType onInit() {
		memset(&NewDeviceName[0],0,sizeof(NewDeviceName));
		IHC.set(&NewDeviceName[0],sizeof(NewDeviceName));
		VKB.init(VirtualKeyBoard::STDKBLowerCase,&IHC,5,DN8App::get().getCanvasWidth()-5,80,RGBColor::WHITE, RGBColor::BLACK,
				RGBColor::BLUE,'_');
		return ErrorType();
	}

	virtual libesp::BaseMenu::ReturnStateContext onRun() {
		BaseMenu *nextState = this;
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);

		VKB.process();

		DN8App::get().getDisplay().drawString(0,10,(const char *)"Current Name: ");
		DN8App::get().getDisplay().drawString(0,20, CurrentDeviceName);
		DN8App::get().getDisplay().drawString(0,30, (const char *)"New Name:");
		DN8App::get().getDisplay().drawString(0,40, &NewDeviceName[0]);
		DN8App::get().getDisplay().drawString(0,60,(const char *)"Mid button finishes");
		if(DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1)) {
			nextState = DN8App::get().getCommunicationSettingState();
		}
		return ReturnStateContext(nextState);
	}

	virtual libesp::ErrorType onShutdown() {
		return ErrorType();
	}
};

static BLESetDeviceNameMenu BLESetName_Menu;

class BLEBoolMenu: public DN8BaseMenu {
public:
	enum TYPE {AD};
private:
	libesp::GUIListData BLEList;
	libesp::GUIListItemData Items[2];
	char ListBuffer[2][6];
	bool YesOrNo;
	TYPE Type;
public:
		BLEBoolMenu(const char *name, TYPE t) : DN8BaseMenu(),	BLEList(name, Items, 0, 0, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0, (sizeof(Items) / sizeof(Items[0]))), YesOrNo(false), Type(t) {
		strcpy(&ListBuffer[0][0],"Yes");
		strcpy(&ListBuffer[1][0],"No");
	}
	virtual ~BLEBoolMenu() {}
	void setValue(bool b) {YesOrNo = b;}
protected:
	virtual libesp::ErrorType onInit() {
		BLEList.items[0].id = 0;
		BLEList.items[0].text = &ListBuffer[0][0];
		BLEList.items[1].id = 1;
		BLEList.items[1].text = &ListBuffer[1][0];
		if(YesOrNo) {
			BLEList.selectedItem = 0;
		} else {
			BLEList.selectedItem = 1;
		}
		return ErrorType();
	}

	virtual libesp::BaseMenu::ReturnStateContext onRun() {
		BaseMenu *nextState = this;
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
		DN8App::get().getGUI().drawList(&BLEList);

		if (!GUIListProcessor::process(&BLEList,(sizeof(Items) / sizeof(Items[0])))) {
			if(DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1)) {
				nextState = DN8App::get().getCommunicationSettingState();
			}
		}
		return ReturnStateContext(nextState);
	}

	virtual libesp::ErrorType onShutdown() {
		return ErrorType();
	}
};
static const char *Advertise = "BLE Advertise";
static BLEBoolMenu BLEAdvertise_Menu(Advertise, BLEBoolMenu::AD);

const char *WIFI_SECURITY[] = {
	"WIFI_AUTH_OPEN",
	"WIFI_AUTH_WEP",
	"WIFI_AUTH_WPA_PSK",
	"WIFI_AUTH_WPA2_PSK",
	"WIFI_AUTH_WPA_WPA2_PSK"
};

static StaticQueue_t TouchMenuQueue;
static const int WIFI_MENU_TOUCH_QUEUE_SIZE = 4;
static const int WIFI_MENU_TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
static uint8_t TouchMenuQueueBuffer[WIFI_MENU_TOUCH_QUEUE_SIZE*WIFI_MENU_TOUCH_MSG_SIZE] = {0};
class WiFiMenu : public DN8BaseMenu {
private:
	VirtualKeyBoard VKB;
	char SSID[17];
	char Password[32];
	VirtualKeyBoard::InputHandleContext IHC;
	libesp::GUIListData WifiSettingList;
	libesp::GUIListItemData Items[7];
	uint16_t WorkingItem;
	static const uint16_t NO_WORKING_TIME = 0xFFFF;
	static const uint16_t ItemCount = uint16_t(sizeof(Items) / sizeof(Items[0]));
	bool APStatus;
	uint16_t SecurityType;
	QueueHandle_t TouchQueueHandle;
public:
	WiFiMenu() : DN8BaseMenu(), VKB(), SSID(), Password(), IHC(0,0), WifiSettingList("WiFi Settings:", Items,0,0, DN8App::get().getLastCanvasWidthPixel()-4, 90, 0, ItemCount), WorkingItem(NO_WORKING_TIME), APStatus(false) { 
		TouchQueueHandle = xQueueCreateStatic(WIFI_MENU_TOUCH_QUEUE_SIZE,WIFI_MENU_TOUCH_MSG_SIZE,&TouchMenuQueueBuffer[0],&TouchMenuQueue);
	}
	void setAPStatus(bool b) {APStatus=b;}
	void setSecurityType(wifi_auth_mode_t &t) {SecurityType=uint16_t(t);}
	void setSSID(const char *ssid) {strncpy(&SSID[0],ssid,sizeof(SSID));}
	virtual ~WiFiMenu() {}
protected:
	virtual libesp::ErrorType onInit() {
		clearListBuffer();
		memset(&SSID[0],0,sizeof(SSID));
		memset(&Password[0],0,sizeof(Password));
		for (uint32_t i = 0;i<ItemCount;++i) {
			Items[i].id = i;
			Items[i].text = getRow(i);
			Items[i].setShouldScroll();
		}
		WorkingItem = NO_WORKING_TIME;
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
		DN8App::get().getTouch().addObserver(TouchQueueHandle);
		return ErrorType();
	}

	virtual libesp::BaseMenu::ReturnStateContext onRun() {
		BaseMenu *nextState = this;
		sprintf(getRow(0),"AP Status: %s", APStatus?"UP":"DOWN");
		if(APStatus) {
			sprintf(getRow(1),"AP Type: %s", WIFI_SECURITY[SecurityType]);
			sprintf(getRow(2),"SSID: %s", &SSID[0]);
			if(SecurityType!=WIFI_AUTH_OPEN ) {
				sprintf(getRow(3),"Password: %s", &Password[0]);
			}
		}
		strcpy(getRow(4),"Submit");

		if(WorkingItem==NO_WORKING_TIME) {
			TouchNotification *pe = nullptr;
			bool penUp = false;
			bool hdrHit = false;
			pe = processTouch(TouchQueueHandle, WifiSettingList, ItemCount, penUp,hdrHit);
			if (pe || !GUIListProcessor::process(&WifiSettingList,ItemCount)) {
				if(penUp ||selectAction()) {
					WorkingItem = WifiSettingList.selectedItem;
					switch(WorkingItem) {
					case 0:
						//do nothing
						strcpy(getRow(6),"Use Fire end this menu");
					break;
					case 1:
						strcpy(getRow(6),"Use Fire end this menu");
					break;
					case 2:
						IHC.set(&SSID[0],sizeof(SSID));
						VKB.init(VirtualKeyBoard::STDKBLowerCase,&IHC,5,DN8App::get().getLastCanvasWidthPixel()-5,100,RGBColor::WHITE,	RGBColor::BLACK, RGBColor::BLUE,'_');
						strcpy(getRow(6),"Press UP/DOWN to esc this menu");
					break;
					case 3:
						IHC.set(&Password[0],sizeof(Password));
						VKB.init(VirtualKeyBoard::STDKBLowerCase,&IHC,5,DN8App::get().getLastCanvasWidthPixel()-5,100,RGBColor::WHITE,	RGBColor::BLACK, RGBColor::BLUE,'_');
						strcpy(getRow(6),"Press UP/DOWN to esc this menu");
					break;
					case 4: {
						if(SSID[0]!='\0') { 
							QueueHandle_t h = nullptr;
							if(APStatus) { //ap down bring it up
								DN8App::get().getWifiTask()->requestAPUp(h,SecurityType,&SSID[0],&Password[0]);
							} else { //ap up bring it down
								DN8App::get().getWifiTask()->requestAPDown(h);
							}
							nextState = DN8App::get().getDisplayMessageState(DN8App::get().getCommunicationSettingState(),(const char *)"Updating ESP",5000);
						} else {
							DN8App::get().getDisplay().drawString(0,80,(const char *)"SID Can't be blank");
						}
						}
					break;
					}
				} else if (backAction() || hdrHit) {
					nextState = DN8App::get().getCommunicationSettingState();
				}
			} 
		} else {
			switch(WorkingItem) {
			case 0:
				if(upAction() || downAction()) {
					APStatus = !APStatus;
				} else if (selectAction()) {
					WorkingItem = NO_WORKING_TIME;
				}
				break;
			case 1:
			{
				if(upAction()) {
					++SecurityType;
					if(SecurityType==WIFI_AUTH_WEP)++SecurityType;
					SecurityType=SecurityType%WIFI_AUTH_MAX;
				} else if (downAction()) {
					if(SecurityType!=WIFI_AUTH_OPEN) {
						--SecurityType;
					}
				} else if (selectAction()) {
					WorkingItem = NO_WORKING_TIME;
				}
			}
			break;
			case 2:
				VKB.process();
				if (backAction()) {
					WorkingItem = NO_WORKING_TIME;
				}
				break;
			case 3:
				VKB.process();
				if (backAction()) {
					WorkingItem = NO_WORKING_TIME;
				}
				break;
			}
		}
		DN8App::get().getGUI().drawList(&WifiSettingList);
		return ReturnStateContext(nextState);
	}

	virtual libesp::ErrorType onShutdown() {
		WifiSettingList.selectedItem=0;
		DN8App::get().getTouch().removeObserver(TouchQueueHandle);
		return ErrorType();
	}
};
static WiFiMenu WiFiMenuInstance;


CommunicationSettingState::CommunicationSettingState() : DN8BaseMenu(), 
	CommSettingList("Comm Info:", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel()
	, DN8App::get().getLastCanvasHeightPixel(), 0, (sizeof(Items) / sizeof(Items[0])))
	, Items(), CurrentDeviceName(), InternalState(NONE)
	{ 

	WifiQueueHandle = xQueueCreateStatic(WIFI_QUEUE_SIZE,WIFI_MSG_SIZE,&WIFIQueueBuffer[0],&WIFIQueue);
	TouchQueueHandle = xQueueCreateStatic(TOUCH_QUEUE_SIZE,TOUCH_MSG_SIZE,&TouchQueueBuffer[0],&TouchQueue);
}

CommunicationSettingState::~CommunicationSettingState() {

}

ErrorType CommunicationSettingState::onInit() {
	clearListBuffer();
	for(int i=0;i<(sizeof(Items)/sizeof(Items[0]));++i) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}
	DN8App::get().getWifiTask()->requestStatus(WifiQueueHandle);
	//sprintf(getRow(1),"BLE Advertise: %s",DN8App::get().getBTTask().isAdvertiseStr());
	sprintf(getRow(2),"BLE Status: %s", ble_get_initialized() ? "Active" : "Inactive");
	sprintf(getRow(3),"BLE DeviceName: %s",DN8App::get().getContacts().getSettings().getAgentName());
	sprintf(getRow(4), "BLE Passkey: %d",ble_get_passkey());

	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(TouchQueueHandle, &pe, 0)) {
			delete pe;
		}
	}
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	DN8App::get().getDisplay().drawString(0,30,(const char *)"***FETCHING DATA***");
	InternalState=FETCHING_DATA;
	return ErrorType();
}

BaseMenu::ReturnStateContext CommunicationSettingState::onRun() {
	BaseMenu *nextState = this;
	if(InternalState==FETCHING_DATA) {
		WIFIResponseMsg *wifimsg;
		if(xQueueReceive(WifiQueueHandle, &wifimsg, 0)) {
			if(wifimsg->getType()==WIFI_STATUS_OK) {
				sprintf(getRow(0),"WifiStatus: %s",
									 wifimsg->getStatusMsg().isApStarted()?"AP UP":"AP DOWN");
				WiFiMenuInstance.setAPStatus(wifimsg->getStatusMsg().isApStarted());
				WiFiMenuInstance.setSSID(wifimsg->getStatusMsg().getSSID());
				DN8App::get().getTouch().addObserver(TouchQueueHandle);
				InternalState=DISPLAY_DATA;
			}
		}
		if(this->getTimesRunCalledSinceLastReset()>250) {
			nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(),"Failed to fetch data",2000);
		}
		delete wifimsg;
	} else {
		TouchNotification *pe = nullptr;
		bool penUp = false;
		bool hdrHit = false;
		pe = processTouch(TouchQueueHandle, CommSettingList, ItemCount, penUp, hdrHit);
		if(pe || !GUIListProcessor::process(&CommSettingList,ItemCount)) {
			if (penUp || selectAction()) {
				DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
				switch(CommSettingList.selectedItem) {
				case 0:
					nextState = &WiFiMenuInstance;
					break;
				}
			} else if(hdrHit) {
				nextState = DN8App::get().getMenuState();
			}
		}
		DN8App::get().getGUI().drawList(&CommSettingList);
	}
	return ReturnStateContext(nextState);
}

ErrorType CommunicationSettingState::onShutdown() {
	DN8App::get().getTouch().removeObserver(TouchQueueHandle);
	return ErrorType();
}


