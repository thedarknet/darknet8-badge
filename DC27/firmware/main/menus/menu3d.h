#ifndef MENU3D_H
#define MENU3D_H

#include "dn8base_menu.h"
#include "3d/renderer.h"
#include "3d/vec_math.h"

namespace libesp {
	class DisplayDevice;
	class RGBColor;
}


class Menu3D : public DN8BaseMenu {
public:
	Menu3D();
	virtual ~Menu3D();
public:
protected:
	virtual libesp::ErrorType onInit();
	virtual libesp::BaseMenu::ReturnStateContext onRun();
	virtual libesp::ErrorType onShutdown();
	void initMenu3d();
	void update();
	void render();
	void line(int x0, int y0, int x1, int y1, libesp::RGBColor& color);
private:
	Model model;
	Vec3f light_dir;
	Vec3f eye;
	static const Vec3f center;
	static const Vec3f up;
	uint8_t CanvasWidth;
	uint8_t CanvasHeight;
};

#endif
