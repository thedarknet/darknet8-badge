#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include "string.h"
#include "ttt3d.h"
#include "game_master.h"

const char* TTT3D_LOGTAG = "TTT3DGame";

bool TTT3DGameTask::verifyInput(GameMsg* msg)
{
	char tx[2], ty[2], tz[2];
	tx[1] = 0;
	ty[1] = 0;
	tz[1] = 0;
	// Validate format (x-y-z)
	// Validate space is not taken
	if (msg->length < 3)
	{
		SendStringResponse(msg, "Invalid command.\n");
		SendStringResponse(msg, "Format: xyz\n");
		return false;
	}
	tx[0] = msg->data[0];
	ty[0] = msg->data[1];
	tz[0] = msg->data[2];
	x = atoi(tx);
	y = atoi(ty);
	z = atoi(tz);
	if (((x < 0) || (x > 2)) || ((y < 0) || (y > 2)) || ((z < 0) || (z > 2)))
	{
		ESP_LOGI(LOGTAG, "x %d y %d z %d", x, y, z);
		SendStringResponse(msg, "Invalid command.\n");
		SendStringResponse(msg, "xyz (0-2 each)\n");
		return false;
	}
	if (this->board[x][y][z] != '_')
	{
		SendStringResponse(msg, "That space is taken.\n");
		return false;
	}
	return true;
}

static char boardline[] = "|%c|%c|%c|\n|%c|%c|%c|\n|%c|%c|%c|\n\n";
void TTT3DGameTask::printBoard(GameMsg* msg)
{
	char* tmpline;
	int level = 0;
	SendStringResponse(msg, "game state\n");
	for (level = 0; level < 3; level++)
	{
		tmpline = (char*)malloc(25);
		sprintf(tmpline, boardline,
			board[0][0][level], board[1][0][level], board[2][0][level],
			board[0][1][level], board[1][1][level], board[2][1][level],
			board[0][2][level], board[0][2][level], board[2][2][level]);
		SendResponse(msg, tmpline, 25);
	}
	return;
}

bool TTT3DGameTask::timeout(GameMsg* msg)
{
	if (!timer_started)
	{
		this->start_time = time(0);
		timer_started = true;
	}
	if ((time(0) - this->start_time) > 100)
	{
		SendStringResponse(msg, "BUZZZZZZZZZZZZZZZZZZZZ\n");
		SendStringResponse(msg, "Time's up! Resetting!\n");
		return true;
	}
	return false;
}

int TTT3DGameTask::scoreBoard(GameMsg* msg)
{
	int score = 0;
	int level = 0;
	// Do the easy ones first
	for (level = 0; level < 3; level++)
	{
		// y lines
		if ((board[0][0][level]==board[0][1][level])&&(board[0][0][level]==board[0][2][level]))
			score += (board[0][0][level] == 'X') ? 1 : -1;
		if ((board[1][0][level]==board[1][1][level])&&(board[1][0][level]==board[1][2][level]))
			score += (board[1][0][level] == 'X') ? 1 : -1;
		if ((board[2][0][level]==board[2][1][level])&&(board[2][0][level]==board[2][2][level]))
			score += (board[2][0][level] == 'X') ? 1 : -1;
		// x lines
		if ((board[0][0][level]==board[1][0][level])&&(board[0][0][level]==board[2][0][level]))
			score += (board[0][0][level] == 'X') ? 1 : -1;
		if ((board[1][0][level]==board[1][1][level])&&(board[1][0][level]==board[1][2][level]))
			score += (board[1][0][level] == 'X') ? 1 : -1;
		if ((board[2][0][level]==board[2][1][level])&&(board[2][0][level]==board[2][2][level]))
			score += (board[2][0][level] == 'X') ? 1 : -1;
		// diagonals
		if ((board[0][0][level]==board[1][1][level])&&(board[0][0][level]==board[2][2][level]))
			score += (board[0][0][level] == 'X') ? 1 : -1;
		if ((board[2][0][level]==board[1][1][level])&&(board[2][0][level]==board[0][2][level]))
			score += (board[2][0][level] == 'X') ? 1 : -1;
	}
	// each spot vertical
	for (x = 0; x < 2; x++)
	{
		for (y = 0; y < 2; y++)
		{
			if ((board[x][y][0] == board[x][y][1]) && (board[x][y][0] == board[x][y][2]))
				score += (board[x][y][0] == 'X') ? 1 : -1;
		}
	}
	// each line vertical diagonals
	if ((board[0][0][0]==board[1][0][1])&&(board[0][0][0]==board[2][0][2])) // tlcd across
		score += (board[0][0][0] == 'X') ? 1 : -1;
	if ((board[0][0][0]==board[0][1][1])&&(board[0][0][0]==board[0][2][2])) // tlcd down
		score += (board[0][0][0] == 'X') ? 1 : -1;
	if ((board[0][0][0]==board[1][1][1])&&(board[0][0][0]==board[2][2][2])) // tlcd diagonal
		score += (board[0][0][0] == 'X') ? 1 : -1;
	if ((board[1][0][0]==board[1][1][1])&&(board[1][0][0]==board[1][2][2])) // tmd down
		score += (board[1][0][0] == 'X') ? 1 : -1;
	if ((board[2][0][0]==board[1][0][1])&&(board[2][0][0]==board[0][0][2])) // trcd across
		score += (board[2][0][0] == 'X') ? 1 : -1;
	if ((board[2][0][0]==board[2][1][1])&&(board[2][0][0]==board[2][2][2])) // trcd down
		score += (board[2][0][0] == 'X') ? 1 : -1;
	if ((board[2][0][0]==board[1][1][1])&&(board[2][0][0]==board[0][2][2])) // trcd diagonal
		score += (board[2][0][0] == 'X') ? 1 : -1;
	if ((board[0][1][0]==board[1][1][1])&&(board[0][1][0]==board[2][1][2])) // mld across
		score += (board[0][1][0] == 'X') ? 1 : -1;
	if ((board[2][1][0]==board[1][1][1])&&(board[2][1][0]==board[0][1][2])) // mrd across
		score += (board[2][1][0] == 'X') ? 1 : -1;
	if ((board[0][2][0]==board[1][2][1])&&(board[0][2][0]==board[2][2][2])) // bld across
		score += (board[0][2][0] == 'X') ? 1 : -1;
	if ((board[0][2][0]==board[0][1][1])&&(board[0][2][0]==board[0][2][2])) // bld up
		score += (board[0][2][0] == 'X') ? 1 : -1;
	if ((board[0][2][0]==board[1][1][1])&&(board[0][2][0]==board[2][0][2])) // bld diagonal
		score += (board[0][2][0] == 'X') ? 1 : -1;
	if ((board[1][2][0]==board[1][1][1])&&(board[1][2][0]==board[1][0][2])) // bmd up
		score += (board[1][2][0] == 'X') ? 1 : -1;
	if ((board[2][2][0]==board[1][2][1])&&(board[2][2][0]==board[0][2][2])) // brd across
		score += (board[2][2][0] == 'X') ? 1 : -1;
	if ((board[2][2][0]==board[2][1][1])&&(board[2][2][0]==board[2][0][2])) // brd up
		score += (board[2][2][0] == 'X') ? 1 : -1;
	if ((board[2][2][0]==board[1][1][1])&&(board[2][2][0]==board[0][0][2])) // brd diagonal
		score += (board[2][2][0] == 'X') ? 1 : -1;

	return score;
}

static char winme[] = "<)935c\x11\x15\x13\x12\x12\x1b\x08\x19\x15\x15\x16\x19\x08\x0b\x01(";
void TTT3DGameTask::makePlay(GameMsg* msg)
{
	// we already verified the input.  this->x,y,z

	// make player move
	this->board[x][y][z] = 'X';
	this->moves++;
	if (moves == 27)
	{
		if (scoreBoard(msg) > 0)
			net_wins++;
		if (net_wins >= 1)
			SendWinResponse(msg, winme);
		this->newGame(msg);
	}
	else
	{
		// ALWAYS take the center space if
		// they don't, it's most valuable
		if (this->board[1][1][1] == '_')
			this->board[1][1][1] = 'O';
		else
		{
			// generate a move for the computer
			while (true)
			{
				x = rand() % 3;
				y = rand() % 3;
				z = rand() % 3;
				if (this->board[x][y][z] == '_')
				{
					this->board[x][y][z] = 'O';
					break;
				}
			}
		}
		this->moves++;
	}

	this->x = -1;
	this->y = -1;
	this->z = -1;
	this->printBoard(msg);
}

void TTT3DGameTask::newGame(GameMsg* msg)
{
	memset(this->board, '_', 27);
	this->moves = 0;
	this->x = -1;
	this->y = -1;
	this->z = -1;
	return;
}

void TTT3DGameTask::reset(GameMsg* msg)
{
	SendStringResponse(msg, "Resetting the game state");
	memset(this->board, '_', 27);
	this->moves = 0;
	this->x = -1;
	this->y = -1;
	this->z = -1;
	this->net_wins = 0;
	start_time = time(0);
	return;
}

void TTT3DGameTask::commandHandler(GameMsg* msg)
{
	ESP_LOGI(LOGTAG, "TTT3D Game Received a Message : %p", msg);

	if (!msg->data)
	{
		SendStringResponse(msg, "WELCOME TO TIC TAC TOE 3D\n");
		SendStringResponse(msg, "Can you beat me 10 times?\n");
		printBoard(msg);
	}
	else if (!strcmp("reset", msg->data))
	{
		reset(msg);
		printBoard(msg);
	}
	else if (timeout(msg))
	{
		reset(msg);
		printBoard(msg);
	}
	else if (verifyInput(msg))
		makePlay(msg);

	return;
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void TTT3DGameTask::run(void* data)
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

bool TTT3DGameTask::init()
{
	this->GameQueueHandle = xQueueCreateStatic(GAME_QUEUE_SIZE, GAME_MSG_SIZE,
		gameQueueBuffer, &GameQueue);
	memset(this->board, '_', 27);
	return true;
}

TTT3DGameTask::TTT3DGameTask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = TTT3D_LOGTAG;
	ESP_LOGI(LOGTAG, "Created");
}

TTT3DGameTask::~TTT3DGameTask()
{
	// TODO:
	ESP_LOGI(LOGTAG, "DESTROY");
}
