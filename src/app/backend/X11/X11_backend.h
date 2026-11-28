#ifndef X11_BACKEND
#define X11_BACKEND

#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../../app/config.h"
#include "../../../app/shader.h"
#include "../../../app/navigation.h"
#include "X11_screenshot.h"

/*********************** MAIN ************************/

typedef unsigned long ID;

typedef struct GL_Backend
{
  GLuint vao, vbo, ebo, texture;
  GLuint shader_program;
	
	void (*delete_buffers)(struct GL_Backend*);
} GL_Backend;

void gl_delete_buffers(GL_Backend *backend);

// I dont know how to name it properly, but XApp seems pretty enough
typedef struct XApp
{
  Config         *config;
  Shader         *vert;
  Shader         *frag;
  Display        *display;
  Window          root_window;
  Window          window;						// NOTE: some of this fields seems useless
  Window          tracking_window; 
  X11_screenshot *screenshot;
  GL_Backend     *gl_backend;
  Atom            delete_message;

  int screen;
  int width, height;
  short rate;

  struct XApp*       (*init)         (Config*, char*, char*, long, bool);
  struct GL_Backend* (*gl_init)      (int, int, Display*, Window, XVisualInfo*, Shader*, Shader*, X11_screenshot*);
  void               (*destroy)      (struct XApp*);
  
  void               (*draw)         (XImage, Camera, Mouse, Flashlight, GLuint, GLuint, GLuint, vec2f, bool);
  int                (*error_handler)(Display*, XErrorEvent*);

	//void							 (*log)					 (char*);
} XApp;

XApp       *XApp_init(Config *config, char *name, char *c_class, long event_mask, bool windowed);
GL_Backend *XApp_gl_init(int w, int h, Display *display, Window window, XVisualInfo *visual_info, Shader *vert, Shader *frag, X11_screenshot *X11_screenshot);
void        XApp_destroy(XApp* app);
void        XApp_draw(XImage image, Camera camera, Mouse mouse, Flashlight flashlight, GLuint shader_program, GLuint vao, GLuint texture, vec2f window_size, bool mirror);
int         XApp_error_handler(Display *display, XErrorEvent *error_event);
//void XApp_log(char *name);

/*********************** UTILITIES ************************/

vec2f get_cursor_position(Display *display);
Window select_window(Display *display);

#endif // X11_BACKEND
