/*
 * gui_list_processor.h
 *
 *  Created on: Jul 16, 2018
 *      Author: dcomes
 */

#ifndef LIBESP_GUI_LIST_PROCESSOR_H_
#define LIBESP_GUI_LIST_PROCESSOR_H_

#include <stdint.h>

namespace libesp {
	class GUIListData;
}

class GUIListProcessor {
public:
	static bool process(libesp::GUIListData *pl, uint16_t itemCount);
};


#endif /* DARKNET_MENUS_GUI_LIST_PROCESSOR_H_ */
