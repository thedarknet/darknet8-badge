#ifndef DARKNET7_NPC_INTERACT
#define DARKNET7_NPC_INTERACT

#include "esp_system.h"
#include "esp_log.h"
#include "lib/Task.h"
#include "freertos/queue.h"
#include <string.h>

// FIXME: Deconflit with other response types??
typedef enum 
{
	NPCI_ACTION_RESP   = 0x00,
	NPCI_NAME_RESP     = 0x01,
} NPCIType;

typedef struct NPCIResponse
{
	NPCIType type;
	uint32_t numstrings;
	char** strings;
	uint32_t infections;
	char* name;
	char* desc;
	char* resp;
	uint8_t wasError;
} NPCIResponse;

class NPCInteractionTask : public Task {
public:
	struct NPCMsg {
		enum REQUEST_TYPE { NONE, HELO, INTERACT };
		REQUEST_TYPE RType;
		QueueHandle_t RetQ;
		char NpcName[32];
		char Action[32];
		NPCMsg(const REQUEST_TYPE &r, QueueHandle_t rq) : RType(r), RetQ(rq), NpcName(), Action() {}
		NPCMsg(const REQUEST_TYPE &r, QueueHandle_t rq, const char *name, const char *action) 
				  : RType(r), RetQ(rq), NpcName(), Action()
		{
			if(name) {
				strcpy(&NpcName[0],name);
			} else {
				memset(&NpcName[0],0,sizeof(NpcName));
			}
			if(action) {
				strcpy(&Action[0],action);
			} else {
				memset(&Action[0],0,sizeof(Action));
			}
		}
	};
	static const int NPCMSG_QUEUE_SIZE = 10;
	static const int NPCMSG_ITEM_SIZE = sizeof(NPCInteractionTask::NPCMsg*);
	static const char *LOGTAG;
public:
	NPCInteractionTask(const std::string &tName, uint16_t stackSize=4196, uint8_t p=5);
	bool init();
	virtual void run(void *data);
	virtual ~NPCInteractionTask();
	QueueHandle_t getQueueHandle() { return InQueueHandle;}
private:
	QueueHandle_t InQueueHandle;
};

#endif
