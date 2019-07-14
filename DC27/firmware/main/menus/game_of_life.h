#ifndef GAME_OF_LIFE_H
#define GAME_OF_LIFE_H

#include "dn8base_menu.h"
#include "libesp/utility/bitarray.h"

class GameOfLife: public DN8BaseMenu {
public:
	GameOfLife();
	virtual ~GameOfLife();
public:
	static const int width = 128;
	static const int height = 128;
	static const int num_slots = width*height;
	static const int sizeof_buffer = (num_slots/8)+1;
	static uint8_t Buffer[sizeof_buffer];
	static const uint32_t NEED_DRAW = 1<<BaseMenu::SHIFT_FROM_BASE;
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
	void initGame();
	bool life(libesp::BitArray &array, char choice, short width, short height, libesp::BitArray &temp);
	enum INTERNAL_STATE {
		INIT, MESSAGE, TIME_WAIT, GAME, SLEEP
	};
	bool shouldDisplayMessage();
private:
	uint16_t Generations;
	uint16_t CurrentGeneration;
	uint8_t Neighborhood;
	libesp::BitArray GOL;
	char UtilityBuf[64];
	INTERNAL_STATE InternalState;
	uint32_t DisplayMessageUntil;
};

#endif
