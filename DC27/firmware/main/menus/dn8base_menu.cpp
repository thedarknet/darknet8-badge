
#include "dn8base_menu.h"

char DN8BaseMenu::ListBuffer[10][32] = {0};
uint8_t DN8BaseMenu::NumRows = 10;

void DN8BaseMenu::clearListBuffer() {
	memset(&ListBuffer[0], 0, sizeof(ListBuffer));
}

char *DN8BaseMenu::getRow(uint8_t row) {
	if(row>=NumRows) return nullptr;
	return &ListBuffer[row][0];
}

