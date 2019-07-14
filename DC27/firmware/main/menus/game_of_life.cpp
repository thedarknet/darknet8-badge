#include "game_of_life.h"
#include <stdlib.h>
#include <libesp/device/display/display_device.h>
#include "menu_state.h"
#include "../app.h"
#include <libesp/freertos.h>
#include "../buttons.h"

using libesp::ErrorType;
using libesp::RGBColor;
using libesp::BitArray;

uint8_t GameOfLife::Buffer[] = {0};
using libesp::FreeRTOS;

GameOfLife::GameOfLife() : DN8BaseMenu(), Generations(0), CurrentGeneration(0), Neighborhood(0), 
	GOL(&Buffer[0],num_slots,1), UtilityBuf(), InternalState(GameOfLife::GAME), DisplayMessageUntil(0) {
}

GameOfLife::~GameOfLife() {

}

ErrorType GameOfLife::onInit() {
	InternalState = INIT;
	return ErrorType();
}

bool GameOfLife::shouldDisplayMessage() {
	return FreeRTOS::getTimeSinceStart() < DisplayMessageUntil;
}

static uint8_t noChange = 0;

libesp::BaseMenu::ReturnStateContext GameOfLife::onRun() {
	switch (InternalState) {
	case INIT: {
		DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
		DisplayMessageUntil = FreeRTOS::getTimeSinceStart() + 3000;
		initGame();
		noChange = 0;
	}
		break;
	case MESSAGE:
		DN8App::get().getDisplay().drawString(0, 10, &UtilityBuf[0], RGBColor::BLACK, RGBColor::WHITE, 1, true);
		InternalState = TIME_WAIT;
		break;
	case TIME_WAIT:
		if (!shouldDisplayMessage()) {
			InternalState = GAME;
			DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
		}
		break;
	case GAME: {
		if (CurrentGeneration >= Generations) {
			InternalState = INIT;
		} else {
			uint16_t count = 0;
			int16_t xOffSet = DN8App::get().getCanvasWidth()-width;
			xOffSet = xOffSet>0? xOffSet/2 : 0;

			//uint8_t bitToCheck = CurrentGeneration % 32;
			for (uint16_t j = 0; j < height; j++) {
				for (uint16_t k = 0; k < width; k++) {
					if(GOL.getValueAsByte((j*width)+k)) {
						DN8App::get().getDisplay().drawPixel(k+xOffSet, j, RGBColor::WHITE);
						count++;
					} else {
						DN8App::get().getDisplay().drawPixel(k+xOffSet, j, RGBColor::BLACK);
					}
				}
			}
			if (0 == count) {
				sprintf(&UtilityBuf[0], "   ALL DEAD\n   After %d\n   generations", CurrentGeneration);
				CurrentGeneration = Generations + 1;
				InternalState = MESSAGE;
				DisplayMessageUntil = FreeRTOS::getTimeSinceStart() + 3000;
				DN8App::get().getDisplay().fillScreen(RGBColor::BLACK);
			} else {
				uint8_t tmpBuffer[sizeof(Buffer)];
				BitArray tmp(&tmpBuffer[0],num_slots,1);
				if (!life(GOL, Neighborhood, width, height, tmp)) {
					noChange++;
					if (noChange > 6) {
						CurrentGeneration = Generations + 1;
					}
				}
			}
			CurrentGeneration++;
		}
	}
		break;
	case SLEEP:
		break;
	}
	libesp::BaseMenu *next = DN8App::get().getMenuState();
	if (!DN8App::get().getButtonInfo().wasAnyButtonReleased()) {
		return ReturnStateContext(this);
	} else {
		return ReturnStateContext(next);
	}
}

ErrorType GameOfLife::onShutdown() {
	return ErrorType();
}

void GameOfLife::initGame() {
	uint32_t start = FreeRTOS::getTimeSinceStart();
	CurrentGeneration = 0;
	Neighborhood = (start & 1) == 0 ? 'm' : 'v';
	srand(start);
	short chanceToBeAlive = rand() % 50;
	memset(&Buffer[0], 0, sizeof(Buffer));
	for (int j = 0; j < height ; j++) {
		for (int i = 0; i < width; i++) {
			if ((rand() % 100) < chanceToBeAlive) {
				//gol[j] |= (1 << i);
				GOL.setValueAsByte((j*width)+i,1);
			}
		}
	}
	Generations = 100 + (rand() % 75);
	InternalState = MESSAGE;
	DisplayMessageUntil = start + 3000;
	sprintf(&UtilityBuf[0], "  Max\n  Generations: %d", Generations);
}
//The life function is the most important function in the program.
//It counts the number of cells surrounding the center cell, and
//determines whether it lives, dies, or stays the same.
bool GameOfLife::life(BitArray &array, char choice, short width, short height, BitArray& temp) {
	//Copies the main array to a temp array so changes can be entered into a grid
	//without effecting the other cells and the calculations being performed on them.
	temp  = array;
	for (int j = 1; j < height - 1; ++j) {
		for (int i = 0; i < width; ++i) {
			int count = 0;
			if (choice == 'm') {
				//The Moore neighborhood checks all 8 cells surrounding the current cell in the array.
				//int count = 0;
				count = (array.getValueAsByte(((j-1)*width)+i)); //up
				if(i>0) {
					count +=(array.getValueAsByte(((j-1)*width)+i-1)); //up to left
					count +=(array.getValueAsByte(((j)*width)+i-1)); //left
					count +=(array.getValueAsByte(((j+1)*width)+i-1)); ///left down
				}
				if(i<(width-1)) {
					count +=(array.getValueAsByte(((j+1)*width)+i+1)); ///down right
					count +=(array.getValueAsByte(((j)*width)+i+1)); ///right
					count +=(array.getValueAsByte(((j-1)*width)+i+1)); ///up right
				}
				count +=(array.getValueAsByte(((j+1)*width)+i)); ///down
			} else if (choice == 'v') {
				//The Von Neumann neighborhood checks only the 4 surrounding cells in the array, (N, S, E, and W).
				count = (array.getValueAsByte(((j-1)*width)+i)); //up
				count +=(array.getValueAsByte(((j)*width)+i-1)); //left
				count +=(array.getValueAsByte(((j+1)*width)+i)); ///down
				count +=(array.getValueAsByte(((j)*width)+i+1)); ///right
			}
			if (count < 2 || count > 3) {
				temp.setValueAsByte((j*width)+i,0);
			}
			if (count == 3) {
				temp.setValueAsByte((j*width)+i,1);
			}
		}
	}
	if (array==temp) {
		return false;
	} else {
		//Copies the completed temp array back to the main array.
		array = temp;
		return true;
	}
}
