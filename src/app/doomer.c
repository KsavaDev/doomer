#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "doomer.h"
#include "navigation.h"

void zoom_in(unsigned int state, long control_mask, Flashlight *flashlight, Camera *camera, Config config, Mouse mouse)
{
  if ((state & control_mask) > 0 && flashlight->is_enabled)
  {
    flashlight->delta_radius += INITIAL_FL_DELTA_RADIUS;
  }
  else 
  {
    camera->delta_scale += config.scroll_speed;
    camera->scale_pivot = mouse.curr;
  }
}

void zoom_out(unsigned int state, long control_mask, Flashlight *flashlight, Camera *camera, Config config, Mouse mouse)
{
  if ((state & control_mask) > 0 && flashlight->is_enabled)
  {
    flashlight->delta_radius -= INITIAL_FL_DELTA_RADIUS;
  }
  else 
  {
    camera->delta_scale -= config.scroll_speed;
    camera->scale_pivot = mouse.curr;
  }
}

WM get_WM()
{
	char *wm = getenv("XDG_SESSION_TYPE");
	printf("getenv(XDG_SESSION_TYPE): %s\n", wm);

	if (strcmp(wm, "X11") == 0)
		return X11;
	if (strcmp(wm, "wayland") == 0)
		return WAYLAND;
	else return DE;
}
