#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include "string.h"
#include "brainfuzz.h"
#include "game_master.h"

const char* BRAINFUZZ_LOGTAG = "BrainfuzzGame";

#define MAX_LINE_BYTES     (80)
#define INSTRUCTION_OFFSET (512-MAX_LINE_BYTES)
#define KEY_OFFSET         (512-MAX_LINE_BYTES-16)

char brainfuzz_flag[] = "flag{MegaSqueep}\0";

void BrainfuzzGameTask::runBrainfuzz(GameMsg* msg)
{
	char *iptr;
	char *dptr;
	int i = 0;
	int numz = 0;

	memset(this->buf, '\0', 512);
	memset(this->output, '\0', 512);
	memcpy(this->buf+KEY_OFFSET, &brainfuzz_flag[0], strlen(brainfuzz_flag));
	memcpy(this->buf+INSTRUCTION_OFFSET, msg->data, msg->length);
	this->output_length = 0;

	// setup the interpreter
	iptr = buf+INSTRUCTION_OFFSET;
	dptr = buf;

	SendStringResponse(msg, "The wheels WHIR into motion\n");
	while (*iptr)
	{
		if (*iptr == '>')
		{
			dptr++;
			if (dptr >= this->buf+INSTRUCTION_OFFSET)
				dptr = this->buf;
		}
		else if (*iptr == '<')
		{
			dptr--;
			if (dptr < this->buf)
				dptr = (this->buf+INSTRUCTION_OFFSET-1);
		}
		else if (*iptr == '+')
			*dptr = (*dptr) + 1;
		else if (*iptr == '-')
			*dptr = (*dptr) - 1;
		else if (*iptr == '.')
		{
			this->output[this->output_length++] = *dptr;
			if (*dptr=='z' && numz >= 0)
				numz++;
			else
				numz-=1;
		}
		else if (*iptr == '[')
		{
			if (*dptr == 0)
			{
				// jump forward
				i = 1;
				while (*iptr && i)
				{
					iptr++;
					if (*iptr == '[')
						i++;
					else if (*iptr == ']')
						i--;
				}
				if (*iptr == 0)
				{
					SendStringResponse(msg, "Errrrrr.... '['");
					break;
				}
			}
		}
		else if (*iptr == ']')
		{
			// Jump back
			i = -1;
			while (*iptr && i)
			{
				iptr--;
				if (*iptr == '[')
					i++;
				else if (*iptr == ']')
					i--;
			}
			if (*iptr == 0)
			{
				SendStringResponse(msg, "Errrrrr.... ']'");
				break;
			}
			continue;
		}
		else if (*iptr == '\0')
			break; // normal end
		iptr++;
	}

	// print the result if there is one
	if (this->output_length)
	{
		SendCopyResponse(msg, this->output, this->output_length);
		SendStringResponse(msg, "\n");
	}

	SendStringResponse(msg, "The gears abruptly CLANK to a stop\n");

	if (numz==250)
	{
		SendStringResponse(msg, "You hear a quiet click\n");
		SendStringResponse(msg, "A set of wheels you hadn't noticed\n");
		SendStringResponse(msg, "set themselves to\n");
		// flag{SQUEEEEEEP}
		SendWinResponse(msg, "<)935c\x1b\x01\x03\x16\x16\x1f\x0c\x14\x12\x04)");
		SendStringResponse(msg, "\nYou feel satisfied.\n");
		SendStringResponse(msg, "It feels like you could do more...\n");
	}

	return;
}

bool VerifyInput(GameMsg* msg)
{
	uint8_t i = 0;

	if (msg->length > 80)
		return false;
	for (i = 0; i < msg->length; i++)
	{
		switch(msg->data[i])
		{
		case '>':
		case '<':
		case '+':
		case '-':
		case '.':
		case '[':
		case ']':
			break;
		default:
			return false;
		}
	}
	return true;
}

void BrainfuzzGameTask::commandHandler(GameMsg* msg)
{
	ESP_LOGI(LOGTAG, "Brainfuzz Game Received a Message : %p", msg);

	if (!msg->data)
	{
		SendStringResponse(msg, "You see a set a GEARS sitting on a common spindle.\n");
		SendStringResponse(msg, "Each appear to have 7 teeth labled \"> < + - . [ ]\"\n");
		SendStringResponse(msg, "Ye shall not exceed 80 doodles...\n");
		SendStringResponse(msg, "");
		SendStringResponse(msg, "");
	}
	else if (VerifyInput(msg))
		runBrainfuzz(msg);
	else
		SendStringResponse(msg, "THE SPELL IS INVALID. 80 instr limit. \"> < + - . [ ]\"\n");

	return;
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void BrainfuzzGameTask::run(void* data)
{
	GameMsg* msg = nullptr;
	while(1)
	{
		if (xQueueReceive(this->GameQueueHandle, &msg, CmdQueueTimeout))
		{
			if (msg != nullptr)
			{
				this->commandHandler(msg);
				free(msg->data);
				free(msg);
			}
		}
	}
}

bool BrainfuzzGameTask::init()
{
	this->GameQueueHandle = xQueueCreateStatic(GAME_QUEUE_SIZE, GAME_MSG_SIZE,
		gameQueueBuffer, &GameQueue);
	return true;
}

BrainfuzzGameTask::BrainfuzzGameTask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = BRAINFUZZ_LOGTAG;
	ESP_LOGI(LOGTAG, "Created");
}

BrainfuzzGameTask::~BrainfuzzGameTask()
{
	// TODO:
	ESP_LOGI(LOGTAG, "DESTROY");
}
