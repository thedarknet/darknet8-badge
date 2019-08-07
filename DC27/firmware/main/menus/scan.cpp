/*
 * scan.cpp
 *
 *      Author: libesp
 */



#include "scan.h"
#include "menu_state.h"
#include "gui_list_processor.h"
#include "../app.h"
#include <device/touch/XPT2046.h>
#include "../wifi.h"
#include "app/display_message_state.h"

using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::TouchNotification;

static const char *NPCINTERACTION = "NPC Interaction";
static const char *NPCList = "NPC List";

static const int NPC_WIFI_QUEUE_SIZE = 4;
static const int NPC_WIFI_MSG_SIZE = sizeof(WIFIResponseMsg*);
static const int NPC_TOUCH_QUEUE_SIZE = 8;
static const int NPC_TOUCH_MSG_SIZE = sizeof(libesp::TouchNotification*);
static StaticQueue_t NPCWIFIQueue;
static uint8_t NPCWIFIQueueBuffer[NPC_WIFI_QUEUE_SIZE*NPC_WIFI_MSG_SIZE] = {0};
static StaticQueue_t NPCTouchQueue;
static uint8_t NPCTouchQueueBuffer[NPC_TOUCH_QUEUE_SIZE*NPC_TOUCH_MSG_SIZE] = {0};
///////////////////////////////////////////
class NPCInteract: public DN8BaseMenu {
public:
	enum INTERNAL_STATE {NONE, NPC_LIST_REQUEST, NPC_LIST_DISPLAY, NPC_ERROR,
		NPC_INTERACT_REQUEST, NPC_INTERACT_DISPLAY};

private:
	libesp::GUIListData DisplayList;
	libesp::GUIListItemData Items[8];
	INTERNAL_STATE InternalState;
	uint32_t Timer;
	char NPCName[32];
	char SSID[33];
	uint8_t BSSID[6];
	QueueHandle_t WifiQueueHandle;
	QueueHandle_t TouchQueueHandle;
public:
	static const uint16_t ItemCount = (sizeof(Items)/sizeof(Items[0]));
public:
	NPCInteract() : DN8BaseMenu(), DisplayList(NPCList, Items, 0, 0
		, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0, ItemCount), InternalState(NONE), Timer(0), NPCName(), SSID(), BSSID(), WifiQueueHandle(), TouchQueueHandle() {
		WifiQueueHandle = xQueueCreateStatic(NPC_WIFI_QUEUE_SIZE,NPC_WIFI_MSG_SIZE,&NPCWIFIQueueBuffer[0],&NPCWIFIQueue);
		TouchQueueHandle = xQueueCreateStatic(NPC_TOUCH_QUEUE_SIZE,NPC_TOUCH_MSG_SIZE,&NPCTouchQueueBuffer[0],&NPCTouchQueue);
	}
	virtual ~NPCInteract() {}
	void setInteractionSSID(const char *ssid) {
		strncpy(&SSID[0],ssid,sizeof(SSID));
	}
	void setInteractionBSSID(const uint8_t *bssid) {
		memcpy(&BSSID[0],bssid,sizeof(BSSID));
	}
protected:
	virtual libesp::ErrorType onInit() {
		InternalState = NPC_LIST_REQUEST;
		memset(&NPCName[0],0,sizeof(NPCName));
		//memset(&BSSID[0],0,sizeof(BSSID));
		DN8App::get().getWifiTask()->requestNPCList(WifiQueueHandle,&SSID[0],&BSSID[0]);

		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
		DN8App::get().getDisplay().drawString(5,10,(const char *)"Getting NPCs at \nthis location",RGBColor::BLUE);

		return ErrorType();
	}

	virtual libesp::BaseMenu::ReturnStateContext onRun() {
		BaseMenu *nextState = this;
/*
			switch(InternalState) {
			case NPC_LIST_REQUEST:
				if(this->getTimesRunCalledSinceLastReset()>500) {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getDisplayMenuState(), DN8App::NO_DATA_FROM_ESP,2000);
				}
				break;
			case NPC_INTERACT_REQUEST:
				if((HAL_GetTick()-Timer)>8000) {
					nextState = DN8App::get().getDisplayMessageState(DN8App::get().getDisplayMenuState(), DN8App::NO_DATA_FROM_ESP,2000);
				}
				break;
			case NPC_LIST_DISPLAY:
				if (!GUIListProcessor::process(&DisplayList,DisplayList.ItemsCount)) {
					if(DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID)) {
						nextState = DN8App::get().getDisplayMenuState();
					} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_FIRE1)) {
						InternalState = NPC_INTERACT_REQUEST;
						strcpy(&NPCName[0],DisplayList.items[DisplayList.selectedItem].text);
						flatbuffers::FlatBufferBuilder fbb;
						std::vector<uint8_t> bssid;
						for(int kk=0;kk<6;kk++) bssid.push_back(InteractInfo->Bssid[kk]);
						auto r = darknet7::CreateWiFiNPCInteractDirect(fbb,&bssid,(const char *)&InteractInfo->Sid[0],
								(int8_t)1, &NPCName[0],0);
						RequestID = DN8App::get().nextSeq();
						auto e = darknet7::CreateSTMToESPRequest(fbb,RequestID,darknet7::STMToESPAny_WiFiNPCInteract, r.Union());
						darknet7::FinishSizePrefixedSTMToESPRequestBuffer(fbb,e);
						const MSGEvent<darknet7::NPCInteractionResponse> *si = 0;
						MCUToMCU::get().getBus().addListener(this,si,&MCUToMCU::get());
						DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);

						DN8App::get().getDisplay().drawString(5,10,(const char *)"Sending Message...",RGBColor::BLUE);

						MCUToMCU::get().send(fbb);
						Timer = HAL_GetTick();
					} else {
						DN8App::get().getGUI().drawList(&DisplayList);
					}
				} else {
					DN8App::get().getGUI().drawList(&DisplayList);
				}
				break;
			case NPC_INTERACT_DISPLAY:
				if (!GUIListProcessor::process(&DisplayList,DisplayList.ItemsCount)) {
					if(DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID)) {
						nextState = DN8App::get().getDisplayMenuState();
					} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_FIRE1)
							&& DisplayList.selectedItem>2 && DisplayList.selectedItem<7) { //hacky hacky hacky
						InternalState = NPC_INTERACT_REQUEST;
						flatbuffers::FlatBufferBuilder fbb;
						std::vector<uint8_t> bssid;
						for(int kk=0;kk<6;kk++) bssid.push_back(InteractInfo->Bssid[kk]);
						auto r = darknet7::CreateWiFiNPCInteractDirect(fbb,&bssid,(const char *)&InteractInfo->Sid[0],
							(int8_t)1, &NPCName[0], DisplayList.items[DisplayList.selectedItem].text);
						RequestID = DN8App::get().nextSeq();
						auto e = darknet7::CreateSTMToESPRequest(fbb,RequestID,darknet7::STMToESPAny_WiFiNPCInteract, r.Union());
						darknet7::FinishSizePrefixedSTMToESPRequestBuffer(fbb,e);
						const MSGEvent<darknet7::NPCInteractionResponse> *si = 0;
						MCUToMCU::get().getBus().addListener(this,si,&MCUToMCU::get());
						DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);

						DN8App::get().getDisplay().drawString(5,10,(const char *)"Sending Message...",RGBColor::BLUE);

						MCUToMCU::get().send(fbb);
						Timer = HAL_GetTick();
					} else {
						DN8App::get().getGUI().drawList(&DisplayList);
					}
				} else {
					DN8App::get().getGUI().drawList(&DisplayList);
				}
				break;
			case NONE:
				break;
			case NPC_ERROR:
				nextState = DN8App::get().getDisplayMessageState(DN8App::get().getDisplayMenuState(),
						(const char *)"Communication error\nWith NPC",(uint16_t)1500);
				break;
			default:
				break;
			}
*/
			return ReturnStateContext(nextState);
	}

	virtual libesp::ErrorType onShutdown() {
		return ErrorType();
	}
};

static NPCInteract NPCInteraction;


////////////////////////////////////////////
static StaticQueue_t WIFIQueue;
static uint8_t WIFIQueueBuffer[Scan::WIFI_QUEUE_SIZE*Scan::WIFI_MSG_SIZE] = {0};
static StaticQueue_t TouchQueue;
static uint8_t TouchQueueBuffer[Scan::TOUCH_QUEUE_SIZE*Scan::TOUCH_MSG_SIZE] = {0};

Scan::Scan() : DN8BaseMenu(), NPCOnly(false),
	DisplayList("WiFi:", Items, 0, 0, DN8App::get().getLastCanvasWidthPixel()
	, DN8App::get().getLastCanvasHeightPixel(), 0, ItemCount), InternalState(NONE)
	, WifiQueueHandle(), TouchQueueHandle(), ResponseMsg(0)  {
	
	WifiQueueHandle = xQueueCreateStatic(WIFI_QUEUE_SIZE,WIFI_MSG_SIZE,&WIFIQueueBuffer[0],&WIFIQueue);
	TouchQueueHandle = xQueueCreateStatic(TOUCH_QUEUE_SIZE,TOUCH_MSG_SIZE,&TouchQueueBuffer[0],&TouchQueue);
}

Scan::~Scan() {

}

ErrorType Scan::onInit() {
	InternalState = FETCHING_DATA;
	ResponseMsg = 0;

	for(int i=0;i<(sizeof(Items)/sizeof(Items[0]));++i) {
		Items[i].text = getRow(i);
		Items[i].id = i;
		Items[i].setShouldScroll();
	}

	DN8App::get().getWifiTask()->requestWifiScan(WifiQueueHandle,isNPCOnly());

	if(isNPCOnly()) {
		DN8App::get().getDisplay().drawString(5,10,(const char *)"Scanning for DarkNet NPCs",RGBColor::BLUE);
	} else {
		DN8App::get().getDisplay().drawString(5,10,(const char *)"Scanning for APs",RGBColor::BLUE);
	}

	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
	return ErrorType();
}

BaseMenu::ReturnStateContext Scan::onRun() {
	BaseMenu *nextState = this;

	if(FETCHING_DATA==InternalState) {
		WIFIResponseMsg *wifimsg;
		if(xQueueReceive(WifiQueueHandle, &wifimsg, 0)) {
			if(wifimsg->getType()==WIFI_SCAN_RESP) {
				for(int i=0;i<wifimsg->getScanResult().RealCount;++i) {
					sprintf(getRow(i),"%s : %d"
						,&wifimsg->getScanResult().ResultArray[i].ssid[0]
						,int32_t(wifimsg->getScanResult().ResultArray[i].rssi));
				}
				DN8App::get().getTouch().addObserver(TouchQueueHandle);
				InternalState=DISPLAY_DATA;
			}
			ResponseMsg = wifimsg;
		}
		if(this->getTimesRunCalledSinceLastReset()>250) {
			nextState = DN8App::get().getDisplayMessageState(DN8App::get().getMenuState(),"Failed to fetch data",2000);
		}
	} else if(DISPLAY_DATA==InternalState) {
		TouchNotification *pe = nullptr;
		bool penUp = false;
		bool hdrHit = false;
		pe = processTouch(TouchQueueHandle, DisplayList, ItemCount, penUp, hdrHit);
		if(pe || !GUIListProcessor::process(&DisplayList,ItemCount)) {
			if(backAction() || hdrHit) {
				nextState = DN8App::get().getMenuState();
			} else if (penUp || selectAction()) {
				NPCInteraction.setInteractionSSID(
				 ResponseMsg->getScanResult().ResultArray[DisplayList.selectedItem].ssid);
				NPCInteraction.setInteractionBSSID((const uint8_t *)
				 ResponseMsg->getScanResult().ResultArray[DisplayList.selectedItem].bssid);
				nextState = &NPCInteraction;
			}
		}
		DN8App::get().getGUI().drawList(&DisplayList);
	}
	return ReturnStateContext(nextState);
}

ErrorType Scan::onShutdown() {
	DN8App::get().getTouch().removeObserver(TouchQueueHandle);
	delete ResponseMsg;
	ResponseMsg = 0;
	return ErrorType();
}

