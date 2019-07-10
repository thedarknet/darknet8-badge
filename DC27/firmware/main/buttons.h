/*
 * DC26.h
 *
 *  Created on: Dec 3, 2017
 *      Author: cmdc0de
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_


class ButtonInfo {
public:
	enum BUTTON {
		BUTTON_LEFT_UP = 0x01
		, BUTTON_RIGHT_DOWN = 0x02
		, BUTTON_FIRE1 = 0x20
		, ANY_KEY = BUTTON_LEFT_UP|BUTTON_RIGHT_DOWN|BUTTON_FIRE1
	};
public:
	ButtonInfo();
	bool init();
	bool areTheseButtonsDown(const int32_t &b);
	bool isAnyOfTheseButtonDown(const int32_t &b);
	bool isAnyButtonDown();
	bool wereTheseButtonsReleased(const int32_t &b);
	bool wereAnyOfTheseButtonsReleased(const int32_t &b);
	bool wasAnyButtonReleased();
	void reset();
	uint32_t lastTickButtonPushed();
	void process();
private:
	uint8_t ButtonState;
	uint8_t LastButtonState;
	uint32_t LastTickButtonPushed;
};

#endif
