#ifndef DOOMER_H
#define DOOMER_H

// This is a main file (dommer/(.c, .h)) where it checks what Window System is installed on machine and by this information uses different method for scrabbing image from screen and providing it to application for zooming.

#include "../util/la.h"
#include "navigation.h"

typedef enum _WM
{
	X11,
	WAYLAND,
	DE,
} WM;

void zoom_in(unsigned int state, long control_mask, Flashlight *flashlight, Camera *camera, Config config, Mouse mouse);
void zoom_out(unsigned int state, long control_mask, Flashlight *flashlight, Camera *camera, Config config, Mouse mouse);

WM get_WM();

#endif // DOOMER_H
