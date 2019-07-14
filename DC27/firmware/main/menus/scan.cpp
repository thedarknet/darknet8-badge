/*
 * scan.cpp
 *
 *      Author: libesp
 */



#include "scan.h"
#include "menu_state.h"
#include "gui_list_processor.h"
#include "../app.h"

using libesp::RGBColor;
using libesp::ErrorType;
using libesp::BaseMenu;

static const char *NPCINTERACTION = "NPC Interaction";
static const char *NPCList = "NPC List";

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
public:
	NPCInteract() : DN8BaseMenu(), DisplayList(NPCList, Items, 0, 0
		, DN8App::get().getLastCanvasWidthPixel(), DN8App::get().getLastCanvasHeightPixel(), 0
		, (sizeof(Items) / sizeof(Items[0]))), InternalState(NONE), Timer(0), NPCName() {

	}
	virtual ~NPCInteract() {}
protected:
	virtual libesp::ErrorType onInit() {
		InternalState = NPC_LIST_REQUEST;
/*
		memset(&NPCName[0],0,sizeof(NPCName));
		flatbuffers::FlatBufferBuilder fbb;
		std::vector<uint8_t> bssid;
		for(int kk=0;kk<6;kk++) bssid.push_back(InteractInfo->Bssid[kk]);
		auto r = darknet7::CreateWiFiNPCInteractDirect(fbb,&bssid,(const char *)&InteractInfo->Sid[0],
				(int8_t)0);
		RequestID = DN8App::get().nextSeq();
		auto e = darknet7::CreateSTMToESPRequest(fbb,RequestID,darknet7::STMToESPAny_WiFiNPCInteract, r.Union());
		darknet7::FinishSizePrefixedSTMToESPRequestBuffer(fbb,e);
		memset(&ListBuffer[0], 0, sizeof(ListBuffer));
		const MSGEvent<darknet7::NPCList> *si = 0;
		MCUToMCU::get().getBus().addListener(this,si,&MCUToMCU::get());
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);

		DN8App::get().getDisplay().drawString(5,10,(const char *)"Getting NPCs at \nthis location",RGBColor::BLUE);

		MCUToMCU::get().send(fbb);
*/
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

Scan::Scan() : DN8BaseMenu(), NPCOnly(false)
	, DisplayList("WiFi:", Items, 0, 0, 160, 128, 0, (sizeof(Items) / sizeof(Items[0])))
	, InternalState(NONE)  {

}

Scan::~Scan() {

}

ErrorType Scan::onInit() {
	InternalState = FETCHING_DATA;
/*
	memset(&Wifis[0],0,sizeof(Wifis));
	flatbuffers::FlatBufferBuilder fbb;
	darknet7::WiFiScanFilter filter = this->isNPCOnly()?darknet7::WiFiScanFilter_NPC:darknet7::WiFiScanFilter_ALL;

	auto r = darknet7::CreateWiFiScan(fbb,filter);
	ESPRequestID = DN8App::get().nextSeq();
	auto e = darknet7::CreateSTMToESPRequest(fbb,ESPRequestID,darknet7::STMToESPAny_WiFiScan, r.Union());
	darknet7::FinishSizePrefixedSTMToESPRequestBuffer(fbb,e);
	memset(&ListBuffer[0], 0, sizeof(ListBuffer));
	const MSGEvent<darknet7::WiFiScanResults> *si = 0;
	MCUToMCU::get().getBus().addListener(this,si,&MCUToMCU::get());
	DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);

	if(isNPCOnly()) {
		DN8App::get().getDisplay().drawString(5,10,(const char *)"Scanning for DarkNet NPCs",RGBColor::BLUE);
	} else {
		DN8App::get().getDisplay().drawString(5,10,(const char *)"Scanning for APs",RGBColor::BLUE);
	}

	MCUToMCU::get().send(fbb);
*/
	return ErrorType();
}

BaseMenu::ReturnStateContext Scan::onRun() {
	BaseMenu *nextState = this;
/*
	switch(InternalState) {
	case FETCHING_DATA:
		if(this->getTimesRunCalledSinceLastReset()>200) {
			nextState = DN8App::get().getDisplayMessageState(DN8App::get().getDisplayMenuState(), DN8App::NO_DATA_FROM_ESP,2000);
		}
		break;
	case DISPLAY_DATA:
		if (!GUIListProcessor::process(&DisplayList,(sizeof(Items) / sizeof(Items[0])))) {
			if(DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_MID)) {
				nextState = DN8App::get().getDisplayMenuState();
			} else if (DN8App::get().getButtonInfo().wereAnyOfTheseButtonsReleased(DN8App::ButtonInfo::BUTTON_FIRE1)) {
				NPCInteraction.setInteraction(&Wifis[DisplayList.selectedItem]);
				nextState = &NPCInteraction;
			}
		}
		DN8App::get().getGUI().drawList(&DisplayList);
		break;
	case NONE:
		break;
	}
*/
	return ReturnStateContext(nextState);
}


ErrorType Scan::onShutdown() {
	return ErrorType();
}

