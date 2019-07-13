/*
 * communications_settings.cpp
 *
 *  Created on: May 29, 2018
 *      Author: dcomes
 */


#include "communications_settings.h"
#include "gui_list_processor.h"
#include "menu_state.h"

using libesp::ErrorType;
using libesp::RGBColor;
using libesp::BaseMenu;


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
		VKB.init(VirtualKeyBoard::STDKBLowerCase,&IHC,5,DN8App::DISPLAY_WIDTH-5,80,RGBColor::WHITE, RGBColor::BLACK,
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
		if(DN8App::get().getButtonInfo().wereTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_FIRE1)) {
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
	BLEBoolMenu(const char *name, TYPE t) : DN8BaseMenu(),
		BLEList(name, Items, 0, 0, DN8App::DISPLAY_WIDTH, DN8App::DISPLAY_HEIGHT, 0, (sizeof(Items) / sizeof(Items[0])))
		, YesOrNo(false), Type(t) {
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
			if(DN8App::get().getButtonInfo().wereTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_FIRE1)) {
				}
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


class WiFi: public DN8BaseMenu {
private:
	VirtualKeyBoard VKB;
	char SSID[17];
	char Password[32];
	darknet7::WifiMode SecurityType;
	VirtualKeyBoard::InputHandleContext IHC;
	libesp::GUIListData WifiSettingList;
	libesp::GUIListItemData Items[5];
	char ListBuffer[5][33];
	uint16_t WorkingItem;
	//darknet7::WiFiStatus CurrentWiFiStatus;
	static const uint16_t NO_WORKING_TIME = 0xFFFF;
public:
	WiFi() : Darknet7BaseState(), VKB(), SSID(), Password(), SecurityType(darknet7::WifiMode_OPEN), IHC(0,0)
		, WifiSettingList("WiFi Settings:", Items,0,0, DN8App::getCanvasLastWidthPixel(), 70, 0
		, (sizeof(Items) / sizeof(Items[0]))) , ListBuffer(), WorkingItem(NO_WORKING_TIME)
		 { //, CurrentWiFiStatus(darknet7::WiFiStatus_DOWN) {
	}
	//void setWifiStatus(darknet7::WiFiStatus c) {CurrentWiFiStatus = c;}
	virtual ~WiFi() {}
protected:
	virtual libesp::ErrorType onInit() {
		memset(&SSID[0],0,sizeof(SSID));
		memset(&Password[0],0,sizeof(Password));
		memset(&ListBuffer[0],0,sizeof(ListBuffer));
		for (uint32_t i = 0; i < (sizeof(Items) / sizeof(Items[0])); i++) {
			Items[i].id = i;
			Items[i].text = &ListBuffer[i][0];
			Items[i].setShouldScroll();
		}
		WorkingItem = NO_WORKING_TIME;
		return ErrorType();
	}

	virtual libesp::BaseMenu::ReturnStateContext onRun() {
		BaseMenu *nextState = this;
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
/*
		//sprintf(&ListBuffer[0][0],"AP Status: %s", darknet7::EnumNameWiFiStatus(CurrentWiFiStatus));
		if(CurrentWiFiStatus==darknet7::WiFiStatus_DOWN) {
			ListBuffer[1][0] = '\0';
			ListBuffer[2][0] = '\0';
			ListBuffer[3][0] = '\0';
		} else {
			//sprintf(&ListBuffer[1][0],"AP Type: %s", darknet7::EnumNameWifiMode(SecurityType));
			sprintf(&ListBuffer[2][0],"SSID: %s", &SSID[0]);
			if(SecurityType==darknet7::WifiMode_OPEN ) {
				ListBuffer[3][0] = '\0';
			} else {
				sprintf(&ListBuffer[3][0],"Password: %s", &Password[0]);
			}
		}
		strcpy(&ListBuffer[4][0],"Submit");

		if(WorkingItem==NO_WORKING_TIME) {
			if (!GUIListProcessor::process(&WifiSettingList,(sizeof(Items) / sizeof(Items[0])))) {
				if(DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_FIRE1)) {
					WorkingItem = WifiSettingList.selectedItem;

					switch(WorkingItem) {
					case 0:
						//do nothing
						break;
					case 1:
						break;
					case 2:
						IHC.set(&SSID[0],sizeof(SSID));
						VKB.init(VirtualKeyBoard::STDKBLowerCase,&IHC,5,DN8App::DISPLAY_WIDTH-5,100,RGBColor::WHITE,	RGBColor::BLACK, RGBColor::BLUE,'_');
						break;
					case 3:
						IHC.set(&Password[0],sizeof(Password));
						VKB.init(VirtualKeyBoard::STDKBLowerCase,&IHC,5,DN8App::DISPLAY_WIDTH-5,100,RGBColor::WHITE,	RGBColor::BLACK, RGBColor::BLUE,'_');
						break;
					case 4: {
							if(SSID[0]!='\0') { // || CurrentWiFiStatus==darknet7::WiFiStatus_DOWN) {
								//flatbuffers::FlatBufferBuilder fbb;
								//flatbuffers::Offset<void> msgOffset;
								//darknet7::STMToESPAny Msg_type = darknet7::STMToESPAny_StopAP;
								//if(CurrentWiFiStatus==darknet7::WiFiStatus_DOWN) {
							///		auto r = darknet7::CreateStopAP(fbb);
							//		msgOffset = r.Union();
							//	} else {
							//		auto r = darknet7::CreateSetupAPDirect(fbb,&SSID[0],&Password[0],SecurityType);
							//		msgOffset = r.Union();
							//		Msg_type = darknet7::STMToESPAny_SetupAP;
							//	}
							//	auto z = darknet7::CreateSTMToESPRequest(fbb,DN8App::get().nextSeq(),Msg_type,msgOffset);
							//	darknet7::FinishSizePrefixedSTMToESPRequestBuffer(fbb,z);
							//	MCUToMCU::get().send(fbb);
								nextState = DN8App::get().getDisplayMessageState(DN8App::get().getCommunicationSettingState(),(const char *)"Updating ESP",5000);
							} else {
								DN8App::get().getDisplay().drawString(0,80,(const char *)"SID Can't be blank");
							}
						}
						break;
					}
				} else if ( DN8App::get().getButtonInfo().wereTheseButtonsReleased(ButtonInfo::BUTTON_LEFT_UP|ButtonInfo::BUTTON_RIGHT_DOWN)) {
					nextState = DN8App::get().getCommunicationSettingState();
				}
			}
		} else {
			switch(WorkingItem) {
			case 0:
				if(DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(ButtonInfo::BUTTON_UP | DN8App::ButtonInfo::BUTTON_DOWN)) {
					if(CurrentWiFiStatus==darknet7::WiFiStatus_DOWN) CurrentWiFiStatus = darknet7::WiFiStatus_AP_STA;
					else CurrentWiFiStatus = darknet7::WiFiStatus_DOWN;
				} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID | DN8App::ButtonInfo::BUTTON_FIRE1)) {
					WorkingItem = NO_WORKING_TIME;
				}
				break;
			case 1:
			{
				if(DN8App::get().getButtonInfo().wereTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_UP)) {
					uint32_t s = (uint32_t)(SecurityType);
					++s;
					s = s%darknet7::WifiMode_MAX;
					SecurityType = (darknet7::WifiMode)s;
				} else if (DN8App::get().getButtonInfo().wereTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_DOWN)) {
					if(SecurityType==darknet7::WifiMode_MIN) {
						SecurityType = darknet7::WifiMode_MAX;
					} else {
						uint32_t s = (uint32_t)(SecurityType);
						--s;
						SecurityType = (darknet7::WifiMode)s;
					}
				} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID
																					| DN8App::ButtonInfo::BUTTON_FIRE1)) {
					WorkingItem = NO_WORKING_TIME;
				}
			}
			break;
			case 2:
				VKB.process();
				if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID)) {
					WorkingItem = NO_WORKING_TIME;
				}
				break;
			case 3:
				VKB.process();
				if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID)) {
					WorkingItem = NO_WORKING_TIME;
				}
				break;
			}
		}
		DN8App::get().getGUI().drawList(&WifiSettingList);
*/
		return ReturnStateContext(nextState);
	}

	virtual libesp::ErrorType onShutdown() {
		WifiSettingList.selectedItem=0;
		return ErrorType();
	}
};
static WiFi WiFiMenu;


CommunicationSettingState::CommunicationSettingState() : DN8BaseMenu(), 
	CommSettingList("Comm Info:", Items, 0, 0, DN8App::get().getCanvasLastWidthPixel()
	, DN8App::get().getCanvasLastHeightPixel(), 0, (sizeof(Items) / sizeof(Items[0])))
	, Items(), ListBuffer(), CurrentDeviceName(), ESPRequestID(0), InternalState(NONE)
	{ //, CurrentWifiStatus(darknet7::WiFiStatus_DOWN) {

}

CommunicationSettingState::~CommunicationSettingState() {

}

ErrorType CommunicationSettingState::onInit() {
/*
	InternalState = FETCHING_DATA;
	flatbuffers::FlatBufferBuilder fbb;
	auto r = darknet7::CreateCommunicationStatusRequest(fbb);
	ESPRequestID = DN8App::get().nextSeq();
	auto e = darknet7::CreateSTMToESPRequest(fbb,ESPRequestID,darknet7::STMToESPAny_CommunicationStatusRequest, r.Union());
	darknet7::FinishSizePrefixedSTMToESPRequestBuffer(fbb,e);
	memset(&ListBuffer[0], 0, sizeof(ListBuffer));
	const MSGEvent<darknet7::CommunicationStatusResponse> *si = 0;
	MCUToMCU::get().getBus().addListener(this,si,&MCUToMCU::get());
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);

	DN8App::get().getDisplay().drawString(5,10,(const char *)"Fetching data from ESP",RGBColor::BLUE);

	MCUToMCU::get().send(fbb);
*/
	return ErrorType();
}

BaseMenu::ReturnStateContext CommunicationSettingState::onRun() {
	BaseMenu *nextState = this;
/*
	if(InternalState==FETCHING_DATA) {
		if(this->getTimesRunCalledSinceLastReset()>200) {
			const MSGEvent<darknet7::CommunicationStatusResponse> *mevt=0;
			MCUToMCU::get().getBus().removeListener(this,mevt,&MCUToMCU::get());
			nextState = DN8App::get().getDisplayMessageState(DN8App::get().getDisplayMenuState(),DN8App::get().NO_DATA_FROM_ESP,2000);
		}
	} else {
		if (!GUIListProcessor::process(&CommSettingList,(sizeof(Items) / sizeof(Items[0])))) {
			if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_FIRE1)) {
				DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
				switch(CommSettingList.selectedItem) {
				case 0:
					WiFiMenu.setWifiStatus(CurrentWifiStatus);
					nextState = &WiFiMenu;
					break;
				case 1:
					nextState = &BLEAdvertise_Menu;
					break;
				case 2:
					BLESetName_Menu.setCurrentNamePtr(&CurrentDeviceName[0]);
					nextState = &BLESetName_Menu;
					break;
				}
			} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID)) {
				nextState = DN8App::get().getDisplayMenuState();
			}
		}
		DN8App::get().getGUI().drawList(&CommSettingList);
	}
*/
	return ReturnStateContext(nextState);
}

ErrorType CommunicationSettingState::onShutdown() {
	return ErrorType();
}


