/*
 * gui_list_processor.h
 *
 *  Created on: Jul 16, 2018
 *      Author: dcomes
 */

#ifndef LIBESP_GUI_LIST_PROCESSOR_H_
#define LIBESP_GUI_LIST_PROCESSOR_H_

#include <stdint.h>
#include <math/point.h>

namespace libesp {
	class GUIListData;
}

class GUIListProcessor {
public:
	static bool process(libesp::GUIListData *pl, uint16_t itemCount);
	//0 no selection
	//1 select
	//2 header hit
	static const int32_t NO_GUI_ITEM_HIT = 0;
	static const int32_t GUI_ITEM_HIT = 1;
	static const int32_t GUI_HEADER_HIT = 2;
	static int32_t process(libesp::Point2Ds &p, libesp::GUIListData *pl, uint16_t itemC);
};


#endif /* DARKNET_MENUS_GUI_LIST_PROCESSOR_H_ */
