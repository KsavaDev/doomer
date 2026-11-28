#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <linux/limits.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "app/config.h"
#include "util/la.h"
#include "app/navigation.h"
#include "app/doomer.h"
#include "app/shader.h"
#include "app/backend/X11/X11_screenshot.h"
#include "app/backend/X11/X11_backend.h"

int main(void)
{
  char *config_file = "config.txt";

  bool windowed = false;
  float delay_sec = 0.0;

  // TODO: implement application arguments

  Config def_conf = default_config;

  if (access(config_file, F_OK) == 0)
  {
    def_conf = config_load(config_file);
  }
  else
  {
    printf("%s doesn't exist. Using default values.\n", config_file);
  }

  printf("Using config: \n");
  config_print(def_conf, "default_config");

  long event_mask = ButtonPressMask 
									| ButtonReleaseMask 
									| KeyPressMask 
									| KeyReleaseMask 
									| PointerMotionMask 
									| ExposureMask 
									| ClientMessage;

  XApp *app = XApp_init(&def_conf, "doomer", "doomer", event_mask, windowed);

  app->gl_init       = XApp_gl_init;
  app->init          = XApp_init;
  app->draw          = XApp_draw;
  app->error_handler = XApp_error_handler;
  app->destroy       = XApp_destroy;

	app->gl_backend->delete_buffers = gl_delete_buffers;

	Display *display           = app->display;
	Window window              = app->window;
	short rate                 = app->rate;
	X11_screenshot *screenshot = app->screenshot;
	Atom delete_message       = app->delete_message;

	GLuint vao            = app->gl_backend->vao;
	GLuint vbo            = app->gl_backend->vbo;
	GLuint texture        = app->gl_backend->texture;
	GLuint shader_program = app->gl_backend->shader_program;

  bool quitting = false;

  Camera *camera = calloc(1, sizeof(Camera));
  camera->scale = 1.0;
  vec2f pos = get_cursor_position(display);

  Mouse *mouse = calloc(1, sizeof(Mouse));
  mouse->curr = pos;
  mouse->prev = pos;
  Flashlight *flashlight = calloc(1, sizeof(Flashlight));
  flashlight->is_enabled = false;
  flashlight->radius = 200.0;
  bool mirror = false;

  float dt = 1.0 / rate;
  Window origin_window;
  int revert_to_return;
  XGetInputFocus(display, &origin_window, &revert_to_return);

  while (!quitting)
  {
    if (!windowed)
    {
      XSetInputFocus(display, window, RevertToParent, CurrentTime);
    }

    XWindowAttributes wa;
    XGetWindowAttributes(display, window, &wa);
    glViewport(0, 0, wa.width, wa.height);

    XEvent event;
    while (XPending(display) > 0)
    {
      XNextEvent(display, &event);

      switch(event.type)
      {
        case MotionNotify:
				{
          mouse->curr = vec2(event.xmotion.x, event.xmotion.y);

          if (mouse->drag)
          {
            vec2f delta = vec2f_sub(world(*camera, mouse->prev), world(*camera, mouse->curr));
            vec2f_add_assign(&camera->position, delta);

            camera->velocity = vec2f_mul_scalar(delta, rate);
          }

          mouse->prev = mouse->curr;
          break;
				}
        case ClientMessage:
				{
          if ((Atom)(event.xclient.data.l[0]) == delete_message)
          {
            quitting = true;
          }
          break;
				}
        case KeyPress:
        {
          KeySym key = XLookupKeysym(&event.xkey, 0);

          switch (key)
          {
						case XK_equal:  
						{
							zoom_in(event.xkey.state, ControlMask, flashlight, camera, def_conf, *mouse); 
							break;
						}
            case XK_minus:
						{
							zoom_out(event.xkey.state, ControlMask, flashlight, camera, def_conf, *mouse);
							break;
						}
            case XK_0:
						{
              camera->scale = 1.0;
              camera->delta_scale = 0.0;
              camera->position = vec2(0.0, 0.0);
              camera->velocity = vec2(0.0, 0.0);
              mirror = false;
              break;
						}
            case XK_q:
            case XK_Escape:
              quitting = true;
              break;
            // TODO: implement XK_r -- reloading shaders
            // TODO: implement XK_m -- mirroring X11_screenshot
            case XK_f:
						{
              flashlight->is_enabled = !flashlight->is_enabled;
              break;
						}
          }
					break;
        }
        case ButtonPress:
				{
          switch(event.xbutton.button)
          {
            case Button1:
						{
              mouse->prev = mouse->curr;
              mouse->drag = true;
              camera->velocity = vec2(0.0, 0.0);
              break;
						}
            case Button4: 
						{
							zoom_in(event.xkey.state, ControlMask, flashlight, camera, def_conf, *mouse);
							break;
						}
            case Button5: 
						{
							zoom_out(event.xkey.state, ControlMask, flashlight, camera, def_conf, *mouse);
							break;
						}
          }
          break;
				}
        case ButtonRelease:
				{
          switch(event.xbutton.button)
          {
            case Button1:
              mouse->drag = false;
              break;
          }
          break;
				}
      }
    }

    camera_update(camera, def_conf, dt, *mouse, vec2(wa.width, wa.height));
    flashlight_update(flashlight, dt);

    XApp_draw(*screenshot->image, 
							*camera, 
							*mouse, 
							*flashlight, 
							shader_program, 
							vao, 
							texture, 
							vec2(wa.width, wa.height), 
							mirror);

    glXSwapBuffers(display, window);
    glFinish();

#ifdef LIVE
    X11_screenshot_refresh(screenshot, display, app->tracking_window);

    int w = screenshot->image->width;
    int h = screenshot->image->height;

    GLfloat vertices[4][5] = 
    {
      {w, 0, 0.0, 1.0, 1.0},
      {w, h, 0.0, 1.0, 0.0},
      {0, h, 0.0, 0.0, 0.0},
      {0, 0, 0.0, 0.0, 1.0}
    };

    GLuint indices[6] = 
    {
      0, 1, 3,
      1, 2, 3
    };

    glBindBuffer(GL_ARRAY_BUFFER, app->gl_backend->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(vertices)), vertices, GL_STATIC_DRAW);
    glTexImage2D(GL_TEXTURE_2D, 
                 0, GL_RGB, 
                 app->screenshot->image->width, app->screenshot->image->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, app->screenshot->image->data);
#endif
  }


  printf("Freed allocated structures.\n");
  XSetInputFocus(display, origin_window, RevertToParent, CurrentTime);
  XSync(display, 0);


  free(mouse);
  free(camera);
  free(flashlight);
  XApp_destroy(app);
}
