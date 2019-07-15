#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include <nvs.h>
#include <nvs_flash.h>

#include "./game_master.h"

const char* GAMEMASTER_LOGTAG = "GameMaster";

#define INSTALLED_GAMES (4)
bool unlocked_games[INSTALLED_GAMES];

// Helpers for sending responses
void SendResponse(GameMsg* msg, char* data, uint8_t size)
{
	// split into chunks?
	GameMsg* omsg = nullptr;
	if (msg->returnQueue)
	{
		omsg = (GameMsg*)malloc(sizeof(GameMsg));
		memset(omsg, '\0', sizeof(GameMsg));
		omsg->length = size;
		omsg->data = data;
		omsg->returnQueue = nullptr;
		omsg->returnContext = msg->returnContext;
		xQueueSend(msg->returnQueue, (void*)&omsg, (TickType_t)100);
	}
	else
		free(data);
	return;
}

void SendStringResponse(GameMsg* msg, const char* stringToCopy)
{
	int size = strlen(stringToCopy);
	char* tmp = (char*)malloc(size);
	memcpy(tmp, stringToCopy, size);
	SendResponse(msg, tmp, size);
	return;
}

void SendCopyResponse(GameMsg* msg, const char* copyme, uint8_t size)
{
	char* tmp = (char*)malloc(size);
	memcpy(tmp, copyme, size);
	SendResponse(msg, tmp, size);
	return;
}

// This function does a simple encoding function on your flag
// that way you don't store your flag in memory as plaintext
// this can be defeated if someone dumps memory and reverses out
// who is calling this function, but that's ok in my opinion
void SendWinResponse(GameMsg* msg, const char* a)
{
	char x[] = {"GOURRY"};
	int i = 0;
	char* b = (char*)malloc(strlen(a)+1);
	for (i = 0; i < strlen(a); i++)
	{
		b[i] = a[i] ^ x[i % 6];
		x[i % 6] = x[i % 6] + 1;
	}
	b[strlen(a)] = '\n';
	SendResponse(msg, b, strlen(a)+1);
}


/*********************************************************
*                     Tic-Tac-Toe 3d                     *
*********************************************************/

bool GameTask::T3DverifyInput(GameMsg* msg)
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
    t3_x = atoi(tx);
    t3_y = atoi(ty);
    t3_z = atoi(tz);
    if (((t3_x < 0) || (t3_x > 2)) || ((t3_y < 0) || (t3_y > 2)) || ((t3_z < 0) || (t3_z > 2)))
    {
        ESP_LOGI(LOGTAG, "x %d y %d z %d", t3_x, t3_y, t3_z);
        SendStringResponse(msg, "Invalid command.\n");
        SendStringResponse(msg, "xyz (0-2 each)\n");
        return false;
    }
    if (this->t3_board[t3_x][t3_y][t3_z] != '_')
    {
        SendStringResponse(msg, "That space is taken.\n");
        return false;
    }
    return true;
}

static char t3_boardline[] = "|%c|%c|%c|\n|%c|%c|%c|\n|%c|%c|%c|\n\n";
void GameTask::T3DprintBoard(GameMsg* msg)
{
    char* tmpline;
    int level = 0;
    SendStringResponse(msg, "game state\n");
    for (level = 0; level < 3; level++)
    {
        tmpline = (char*)malloc(25);
        sprintf(tmpline, t3_boardline,
            t3_board[0][0][level], t3_board[1][0][level], t3_board[2][0][level],
            t3_board[0][1][level], t3_board[1][1][level], t3_board[2][1][level],
            t3_board[0][2][level], t3_board[1][2][level], t3_board[2][2][level]);
        SendResponse(msg, tmpline, 25);
    }
    return;
}

bool GameTask::T3Dtimeout(GameMsg* msg)
{
    if (!t3_timer_started)
    {
        this->t3_start_time = time(0);
        t3_timer_started = true;
    }
    if ((time(0) - this->t3_start_time) > 100)
    {
        SendStringResponse(msg, "BUZZZZZZZZZZZZZZZZZZZZ\n");
        SendStringResponse(msg, "Time's up! Resetting!\n");
        return true;
    }
    return false;
}

int GameTask::T3DscoreBoard(GameMsg* msg)
{
    int score = 0;
    int level = 0;
    // Do the easy ones first
    for (level = 0; level < 3; level++)
    {
        // y lines
        if ((t3_board[0][0][level]==t3_board[0][1][level])&&
			(t3_board[0][0][level]==t3_board[0][2][level]))
            score += (t3_board[0][0][level] == 'X') ? 1 : -1;
        if ((t3_board[1][0][level]==t3_board[1][1][level])&&
			(t3_board[1][0][level]==t3_board[1][2][level]))
            score += (t3_board[1][0][level] == 'X') ? 1 : -1;
        if ((t3_board[2][0][level]==t3_board[2][1][level])&&
			(t3_board[2][0][level]==t3_board[2][2][level]))
            score += (t3_board[2][0][level] == 'X') ? 1 : -1;
        // x lines
        if ((t3_board[0][0][level]==t3_board[1][0][level])&&
			(t3_board[0][0][level]==t3_board[2][0][level]))
            score += (t3_board[0][0][level] == 'X') ? 1 : -1;
        if ((t3_board[1][0][level]==t3_board[1][1][level])&&
			(t3_board[1][0][level]==t3_board[1][2][level]))
            score += (t3_board[1][0][level] == 'X') ? 1 : -1;
        if ((t3_board[2][0][level]==t3_board[2][1][level])&&
			(t3_board[2][0][level]==t3_board[2][2][level]))
            score += (t3_board[2][0][level] == 'X') ? 1 : -1;
        // diagonals
        if ((t3_board[0][0][level]==t3_board[1][1][level])&&
			(t3_board[0][0][level]==t3_board[2][2][level]))
            score += (t3_board[0][0][level] == 'X') ? 1 : -1;
        if ((t3_board[2][0][level]==t3_board[1][1][level])&&
			(t3_board[2][0][level]==t3_board[0][2][level]))
            score += (t3_board[2][0][level] == 'X') ? 1 : -1;
    }
    // each spot vertical
    for (t3_x = 0; t3_x < 2; t3_x++)
    {
        for (t3_y = 0; t3_y < 2; t3_y++)
        {
            if ((t3_board[t3_x][t3_y][0] == t3_board[t3_x][t3_y][1]) &&
				(t3_board[t3_x][t3_y][0] == t3_board[t3_x][t3_y][2]))
                score += (t3_board[t3_x][t3_y][0] == 'X') ? 1 : -1;
        }
    }
    // each line vertical diagonals
    if ((t3_board[0][0][0]==t3_board[1][0][1])&&
		(t3_board[0][0][0]==t3_board[2][0][2])) // tlcd across
        score += (t3_board[0][0][0] == 'X') ? 1 : -1;
    if ((t3_board[0][0][0]==t3_board[0][1][1])&&
		(t3_board[0][0][0]==t3_board[0][2][2])) // tlcd down
        score += (t3_board[0][0][0] == 'X') ? 1 : -1;
    if ((t3_board[0][0][0]==t3_board[1][1][1])&&
		(t3_board[0][0][0]==t3_board[2][2][2])) // tlcd diagonal
        score += (t3_board[0][0][0] == 'X') ? 1 : -1;
    if ((t3_board[1][0][0]==t3_board[1][1][1])&&
		(t3_board[1][0][0]==t3_board[1][2][2])) // tmd down
        score += (t3_board[1][0][0] == 'X') ? 1 : -1;
    if ((t3_board[2][0][0]==t3_board[1][0][1])&&
		(t3_board[2][0][0]==t3_board[0][0][2])) // trcd across
        score += (t3_board[2][0][0] == 'X') ? 1 : -1;
    if ((t3_board[2][0][0]==t3_board[2][1][1])&&
		(t3_board[2][0][0]==t3_board[2][2][2])) // trcd down
        score += (t3_board[2][0][0] == 'X') ? 1 : -1;
    if ((t3_board[2][0][0]==t3_board[1][1][1])&&
		(t3_board[2][0][0]==t3_board[0][2][2])) // trcd diagonal
        score += (t3_board[2][0][0] == 'X') ? 1 : -1;
    if ((t3_board[0][1][0]==t3_board[1][1][1])&&
		(t3_board[0][1][0]==t3_board[2][1][2])) // mld across
        score += (t3_board[0][1][0] == 'X') ? 1 : -1;
    if ((t3_board[2][1][0]==t3_board[1][1][1])&&
		(t3_board[2][1][0]==t3_board[0][1][2])) // mrd across
        score += (t3_board[2][1][0] == 'X') ? 1 : -1;
    if ((t3_board[0][2][0]==t3_board[1][2][1])&&
		(t3_board[0][2][0]==t3_board[2][2][2])) // bld across
        score += (t3_board[0][2][0] == 'X') ? 1 : -1;
    if ((t3_board[0][2][0]==t3_board[0][1][1])&&
		(t3_board[0][2][0]==t3_board[0][2][2])) // bld up
        score += (t3_board[0][2][0] == 'X') ? 1 : -1;
    if ((t3_board[0][2][0]==t3_board[1][1][1])&&
		(t3_board[0][2][0]==t3_board[2][0][2])) // bld diagonal
        score += (t3_board[0][2][0] == 'X') ? 1 : -1;
    if ((t3_board[1][2][0]==t3_board[1][1][1])&&
		(t3_board[1][2][0]==t3_board[1][0][2])) // bmd up
        score += (t3_board[1][2][0] == 'X') ? 1 : -1;
    if ((t3_board[2][2][0]==t3_board[1][2][1])&&
		(t3_board[2][2][0]==t3_board[0][2][2])) // brd across
        score += (t3_board[2][2][0] == 'X') ? 1 : -1;
    if ((t3_board[2][2][0]==t3_board[2][1][1])&&
		(t3_board[2][2][0]==t3_board[2][0][2])) // brd up
        score += (t3_board[2][2][0] == 'X') ? 1 : -1;
    if ((t3_board[2][2][0]==t3_board[1][1][1])&&
		(t3_board[2][2][0]==t3_board[0][0][2])) // brd diagonal
        score += (t3_board[2][2][0] == 'X') ? 1 : -1;
    return score;
}

// {flag:NowWin10In100Sec}
static char t3_winme1[]="<)935c\x06?!\x04:4xa\x1e:ekz\x01=6(";
// {flag:YEEAAAAHBABBBYY}
static char t3_winme10[]="<)935c\x11\x15\x13\x12\x12\x1b\x08\x19\x15\x15\x16\x19\x08\x0b\x01(";
void GameTask::T3DmakePlay(GameMsg* msg)
{
    // make player move
    this->t3_board[t3_x][t3_y][t3_z] = 'X';
    this->t3_moves++;
    if (t3_moves == 27)
    {
        if (T3DscoreBoard(msg) > 0)
        {
            SendWinResponse(msg, t3_winme1);
            t3_net_wins++;
        }
        else
            SendStringResponse(msg, "You Lost\n");
        if (t3_net_wins >= 10)
            SendWinResponse(msg, t3_winme10);
        this->T3DnewGame(msg);
    }
    else
    {
        bool moved = false;
        do
        {
            t3_x = rand() % 3;
            t3_y = rand() % 3;
            t3_z = rand() % 3;
            if (this->t3_board[t3_x][t3_y][t3_z] == '_')
            {
                this->t3_board[t3_x][t3_y][t3_z] = 'O';
                moved = true;
            }
        } while (!moved);
        this->t3_moves++;
    }

    this->t3_x = -1;
    this->t3_y = -1;
    this->t3_z = -1;
    this->T3DprintBoard(msg);
}

void GameTask::T3DnewGame(GameMsg* msg)
{
    memset(this->t3_board, '_', 27);
    this->t3_moves = 0;
    this->t3_x = -1;
    this->t3_y = -1;
    this->t3_z = -1;
    return;
}

void GameTask::T3Dreset(GameMsg* msg)
{
    SendStringResponse(msg, "Resetting the game state");
    memset(this->t3_board, '_', 27);
    this->t3_moves = 0;
    this->t3_x = -1;
    this->t3_y = -1;
    this->t3_z = -1;
    this->t3_net_wins = 0;
    this->t3_start_time = time(0);
    return;
}

void GameTask::T3DcommandHandler(GameMsg* msg)
{
    //ESP_LOGI(LOGTAG, "TTT3D Game Received a Message : %p", msg);
    if (!msg->data)
    {
        SendStringResponse(msg, "WELCOME TO TIC TAC TOE 3D\n");
        SendStringResponse(msg, "intput coordinates: xyz\n");
        T3DprintBoard(msg);
    }
    else if (!strncmp("reset", msg->data, 5))
    {
        T3Dreset(msg);
        T3DprintBoard(msg);
    }
    else if (T3Dtimeout(msg))
    {
        T3Dreset(msg);
        T3DprintBoard(msg);
    }
    else if (T3DverifyInput(msg))
        T3DmakePlay(msg);

    return;
}

bool GameTask::T3Dinit()
{
    memset(this->t3_board, '_', 27);
    this->t3_moves = 0;
    this->t3_x = -1; 
    this->t3_y = -1; 
    this->t3_z = -1; 
    this->t3_net_wins = 0;
    return true;
}



/***************************************************************
*                    Brainfuzz  Game                           *
***************************************************************/
#define BF_MAX_LINE_BYTES     (80)
#define BF_INSTRUCTION_OFFSET (512-BF_MAX_LINE_BYTES)
#define BF_KEY_OFFSET         (512-BF_MAX_LINE_BYTES-16)
//char brainfuzz_flag[] = "flag{MegaSqueep}\0";
char brainfuzz_flag[] = "!+& <\n\" &" "\x14" "62\"\"7:\0";

bool GameTask::BFverifyInput(GameMsg* msg)
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

void GameTask::BFrun(GameMsg* msg)
{
    char *iptr;
    char *dptr;
    int i = 0;
    int numz = 0;

    memset(this->bf_buf, '\0', 512);
    memset(this->bf_output, '\0', 512);
    memcpy(this->bf_buf+BF_KEY_OFFSET, &brainfuzz_flag[0], strlen(brainfuzz_flag));
    memcpy(this->bf_buf+BF_INSTRUCTION_OFFSET, msg->data, msg->length);

    for (i = 0; i < 16; i++)
        this->bf_buf[BF_KEY_OFFSET+i] = this->bf_buf[BF_KEY_OFFSET+i] ^ 0x47;

    this->bf_output_length = 0;

    // setup the interpreter
    iptr = bf_buf+BF_INSTRUCTION_OFFSET;
    dptr = bf_buf;

    SendStringResponse(msg, "The wheels WHIR into motion\n");
    while (*iptr)
    {   
        if (*iptr == '>')
        {
            dptr++;
            if (dptr >= this->bf_buf+BF_INSTRUCTION_OFFSET)
                dptr = this->bf_buf;
        }
        else if (*iptr == '<')
        {
            dptr--;
            if (dptr < this->bf_buf)
                dptr = (this->bf_buf+BF_INSTRUCTION_OFFSET-1);
        }
        else if (*iptr == '+')
            *dptr = (*dptr) + 1;
        else if (*iptr == '-')
            *dptr = (*dptr) - 1;
        else if (*iptr == '.')
        {
            this->bf_output[this->bf_output_length++] = *dptr;
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
    if (this->bf_output_length)
    {
        SendCopyResponse(msg, this->bf_output, this->bf_output_length);
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


void GameTask::BFcommandHandler(GameMsg* msg)
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
    else if (BFverifyInput(msg))
        BFrun(msg);
    else
        SendStringResponse(msg, "THE SPELL IS INVALID. 80 instr limit. \"> < + - . [ ]\"\n");

    return;
}



/***************************************************************
*                    Exploitable Game                          *
***************************************************************/
static void exp_secretLevel(GameMsg* msg)
{
    SendStringResponse(msg, "Welcome to the secret level!\n");
    SendWinResponse(msg, "<)935c25$#)?;!-1&+7"); // {flag:zerpzerpzerp}
    return;
}

static void exp_badLevel(GameMsg* msg)
{
    SendStringResponse(msg, "Bad level.\n");
}

static bool level5_greeting = false;
static void exp_level5(GameMsg* msg)
{
    if (!msg->length || !level5_greeting)
    {   
        SendStringResponse(msg, "That's all Gourry wrote!(?)\n");
        level5_greeting = true;
    }   
    return;
}

static bool level4_greeting = false;
static void exp_level4(GameMsg* msg)
{
    if (!msg->length || !level4_greeting)
    {   
        SendStringResponse(msg, "Level 4\n");
        level4_greeting = true;
    }   
    return;
}
static bool level3_greeting = false;
static void exp_level3(GameMsg* msg)
{
    if (!msg->length || !level3_greeting)
    {
        SendStringResponse(msg, "Level 3\n");
        level3_greeting = true;
    }
    return;
}

static bool level2_greeting = false;
static void exp_level2(GameMsg* msg)
{
    static int password2;
    static int guess;
    static long int start_time2;
    if (!msg->length || !level2_greeting)
    {
        SendStringResponse(msg, "Level 2\n");
        SendStringResponse(msg, "Input the password. 4*[0-9]\n");
        password2 = rand();
        level2_greeting = true;
        return;
    }
    else if ((time(0) - start_time2) > 30)
    {
        SendStringResponse(msg, "TIMED OUT\n");
        SendStringResponse(msg, "Password reset.\n");
        password2 = rand();
        start_time2 = time(0);
        return;
    }
    guess = atoi(msg->data);
    if (guess == password2)
        SendWinResponse(msg, "<)935c\n<?=7\x16<2<)"); // {flag:BlindLuck}
    else
        SendStringResponse(msg, "Nope.\n");
    return;
}
static bool level1_greeting = false;
static void exp_level1(GameMsg* msg)
{
    static char password[8];
    static long int start_time1;
    int result = 0;
    char* return_msg;
    int i = 0;
    if (!msg->length || !level1_greeting)
    {
        SendStringResponse(msg, "Level 1\n");
        SendStringResponse(msg, "Input the password. 8*[a-z]\n");
        SendStringResponse(msg, "You have 30 seconds.\n");
        for (i = 0; i < 8; i++)
            password[i] = (rand() % (122 - 97 + 1)) + 97;
        start_time1 = time(0);
        level1_greeting = true;
        return;
    }
    else if ((time(0) - start_time1) > 30)
    {
        SendStringResponse(msg, "TIMED OUT\n");
        SendStringResponse(msg, "Password reset.\n");
        for (i = 0; i < 8; i++)
            password[i] = (rand() % (122 - 97 + 1)) + 97;
        start_time1 = time(0);
        return;
    }
    //ESP_LOGI(EXPLOITABLE_LOGTAG, "time(0): %ld, start_time1: %ld\n", time(0), start_time1);
    result = strncmp(password, msg->data, 8);
    if (!result)
        SendWinResponse(msg, "<)935c\x12?9#:.0""\x15""8;$2>+%"); //{flag: ZoopityDoopity}
    else
    {
        return_msg = (char*)malloc(5);
        memset(return_msg, '\n', 5);
        sprintf(return_msg, "%d", result);
        SendResponse(msg, return_msg, 5);
    }
    return;
}
static bool level0_greeting = false;
static char exp0_flag[] = "Very Good! Now Echo This: ZOOP";
static void exp_level0(GameMsg* msg)
{
    char* tmp = nullptr;
    char* ftmp = nullptr;
    int tmp_length = 0; 
    if (!msg->length || !level0_greeting)
    {
        SendStringResponse(msg, "Level 0\n");
        SendStringResponse(msg, "You breath... and it echoes.\n");
        SendStringResponse(msg, "Wow you breath loudly.......\n");
        level0_greeting = true;
        return;
    }

    if ((msg->length >= 4) && (!strncmp("ZOOP", msg->data, 4)))
        SendWinResponse(msg, "<)935c\x0f\"32\'\x1d&>08-\x16%=?9,!"); //{flag:GreatGooglyMoogly}

    tmp_length = (strlen(exp0_flag) > msg->length) ? strlen(exp0_flag) : msg->length;
    tmp = (char*)malloc(tmp_length);
    snprintf(tmp, tmp_length, msg->data, exp0_flag);
    ftmp = (char*)malloc(msg->length + 1);
    memcpy(ftmp, tmp, msg->length);
    ftmp[msg->length] = '\n';
    free(tmp);
    SendResponse(msg, ftmp, msg->length + 1);
    return;
}


void GameTask::EXPcommandHandler(GameMsg* msg)
{
    ESP_LOGI(LOGTAG, "Exploitable Game Received a Message : %p", msg);

    if (!msg->data)
    {
        SendStringResponse(msg, "Welcome to the exploitable game\n");
        SendStringResponse(msg, "To select a level, write 'level #'\n");
        SendStringResponse(msg, "Otherwise you will progress from level 0 upward\n");
        SendStringResponse(msg, "There are 5 levels (0-4)\n");
    }   
    else if (!strncmp(msg->data, "level ", 6)) 
    {
        int level_num = 0;
        if (msg->length < 7)
        {
            SendStringResponse(msg, "Invalid level selection\n");
            return;
        }
        level_num = atoi(&msg->data[6]);
        if (level_num < 0 || level_num == 6 || level_num > 8)
        {
            SendStringResponse(msg, "Bad number in level selection\n");
            return;
        }
        else
        {
            SendStringResponse(msg, "Setting level.\n");
            this->exp_cur_level = level_num;
        }   
    }
    ((void (*)(GameMsg*))exp_levels[exp_cur_level])(msg);
    return;
}

void GameTask::EXPinit()
{
    // setup the level pointers, we do it this way so hopefully someone finds
    // the secret level :3
    exp_levels[0] = (void*)exp_level0;
    exp_levels[1] = (void*)exp_level1;
    exp_levels[2] = (void*)exp_level2;
    exp_levels[3] = (void*)exp_level3;
    exp_levels[4] = (void*)exp_level4;
    exp_levels[5] = (void*)exp_level5;
    exp_levels[6] = (void*)exp_badLevel; // unreachable
    exp_levels[7] = (void*)exp_badLevel;
    exp_levels[8] = (void*)exp_secretLevel;
    if (nvs_get_u32(my_nvs_handle, "exp_cur_level", &exp_cur_level) != ESP_OK)
    {
        exp_cur_level = 0;
        nvs_set_u32(my_nvs_handle, "exp_cur_level", exp_cur_level);
    }
    return;
}




/***************************************************************
*                     GameMaster Code                          *
***************************************************************/

char base_tag[] = "game_unlocked";
char game_tag[20];
bool GameTask::isGameUnlocked(uint8_t gameid)
{
	bool unlocked = false;
	if (gameid >= INSTALLED_GAMES)
		return false;
	snprintf(game_tag, 20, "%s%d", base_tag, gameid);
	nvs_get_u8(this->my_nvs_handle, game_tag, (uint8_t*)&unlocked);
	return unlocked;
}

bool GameTask::setGameUnlocked(uint8_t gameid)
{
	if (gameid >= INSTALLED_GAMES)
		return false;
	unlocked_games[gameid] = true;

	snprintf(game_tag, 20, "%s%d", base_tag, gameid);
	nvs_set_u8(this->my_nvs_handle, game_tag, (uint8_t)true);
	nvs_commit(this->my_nvs_handle);
	return true;
}

void GameTask::mainMenuSendResponse(GameMsg* msg, char* data, uint8_t size)
{
	// split into chunks?
	GameMsg* omsg = nullptr;
	if (msg->returnQueue)
	{
		omsg = (GameMsg*)malloc(sizeof(GameMsg));
		memset(omsg, '\0', sizeof(GameMsg));
		omsg->length = size;
		omsg->data = data;
		omsg->returnQueue = nullptr;
		omsg->returnContext = msg->returnContext;
		xQueueSend(msg->returnQueue, (void*)&omsg, (TickType_t)100);
	}
	else
		free(data);
	return;
}

void GameTask::sendBadContextError(GameMsg* msg)
{
	char* odata = (char*)malloc(27);
	memcpy(odata, "Bad Game Context Selection\n", 27);
	mainMenuSendResponse(msg, odata, 28);
	return;
}

void GameTask::sendGameLockedError(GameMsg* msg)
{
	char* odata = (char*)malloc(12);
	memcpy(odata, "Game Locked\n", 12);
	mainMenuSendResponse(msg, odata, 12);
	return;
}

void GameTask::mainMenu(GameMsg* msg)
{
	ESP_LOGI(LOGTAG, "Game Master");

	// TODO: Unlock codes for games

	return;
}

void GameTask::commandHandler(GameMsg* msg)
{
	//ESP_LOGI(LOGTAG, "Message for Game: %x\n", msg->context);
	if ((msg->context >= INSTALLED_GAMES))
	{
		sendBadContextError(msg);
		goto cleanup;
	}
	else if (!isGameUnlocked(msg->context))
	{
		sendGameLockedError(msg);
		goto cleanup;
	}

	if (msg->context == GAMEMASTER_ID)
		mainMenu(msg);
	else if (msg->context == EXPLOITABLE_ID)
		this->EXPcommandHandler(msg);
	else if (msg->context == BRAINFUZZ_ID)
		this->BFcommandHandler(msg);
	else if (msg->context == TTT3D_ID)
		this->T3DcommandHandler(msg);
cleanup:
	free(msg->data);
	free(msg);
	return;
}

bool GameTask::installGame(GameId id, bool unlocked)
{
	ESP_LOGI(LOGTAG, "Installing Game: %d", id);
	if (id > INSTALLED_GAMES)
	{
		ESP_LOGE(LOGTAG, "FAILED TO INSTALL, increment INSTALLED_GAMES in game_master.cpp");
		return false;
	}
	if (unlocked)
		setGameUnlocked(id);

	ESP_LOGI(LOGTAG, "Game Installed");
	return true;
}

#define CmdQueueTimeout ((TickType_t) 1000 / portTICK_PERIOD_MS)
void GameTask::run(void* data)
{
	GameMsg* msg = nullptr;
	while(1)
	{
		if (xQueueReceive(this->GameQueueHandle, &msg, CmdQueueTimeout))
		{
			if (msg != nullptr)
				this->commandHandler(msg);
		}
	}
}

bool GameTask::init()
{
	int i = 0;
	bool unlocked;
	this->GameQueueHandle = xQueueCreateStatic(GAME_QUEUE_SIZE, GAME_MSG_SIZE,
		gameQueueBuffer, &GameQueue);

	memset(unlocked_games, '\0', sizeof(unlocked_games));
	unlocked_games[0] = true; // GameMaster Menu is always unlocked

	// retrieve locked status
	if (nvs_open("storage", NVS_READWRITE, &this->my_nvs_handle) != ESP_OK)
	{
		ESP_LOGE(LOGTAG, "FAILED TO OPEN NVS");
		return false;
	}

	for (i = 0; i < INSTALLED_GAMES; i++)
	{
		if (i == 0) continue; // ignore game-master, always unlocked
		unlocked = false;
		snprintf(game_tag, 20, "%s%d", base_tag, i);
		if (nvs_get_u8(this->my_nvs_handle, game_tag, (uint8_t*)&unlocked) == ESP_OK)
		{
			ESP_LOGI(LOGTAG, "game_tag: %s. unlocked %d", game_tag, unlocked);
			unlocked_games[i] = unlocked;
		}
		else
		{
			unlocked_games[i] = false;
			nvs_set_u8(this->my_nvs_handle, game_tag, (uint8_t)unlocked);
			nvs_commit(this->my_nvs_handle);
		}
	}

	EXPinit();
	installGame(EXPLOITABLE_ID, true);

	installGame(BRAINFUZZ_ID,   true);

	T3Dinit();
	installGame(TTT3D_ID,       true);

	return true;
}

GameTask::GameTask(const std::string &tName, uint16_t stackSize, uint8_t priority)
	: Task(tName, stackSize, priority)
{
	this->LOGTAG = GAMEMASTER_LOGTAG;
	ESP_LOGI(LOGTAG, "Created");
}

GameTask::~GameTask()
{
	// TODO:
	ESP_LOGI(LOGTAG, "DESTROY");
}
