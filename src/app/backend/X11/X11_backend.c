#define GL_GLEXT_PROTOTYPES
#include "X11_backend.h"
#include "X11_screenshot.h"

GL_Backend *XApp_gl_init(int w, int h, Display *display, Window window, XVisualInfo *visual_info, Shader *vert, Shader *frag, X11_screenshot *X11_screenshot);
void XApp_log(char *name);

XApp *XApp_init(Config *config, char *name, char *c_class, long event_mask, bool windowed)
{
	printf("[XApp]: Initializing XApp.\n");
  XApp *app = calloc(1, sizeof(XApp));
  if (app == NULL)
  {
    fprintf(stderr, "Failed to calloc XApp.\n");
    exit(1);
  }

	Display *display = XOpenDisplay(NULL);
  if (display == NULL)
  {
    fprintf(stderr, "Failed to open display.\n");
    exit(1);
  }

  XSetErrorHandler(XApp_error_handler);

  Window root_window = DefaultRootWindow(display);
  // TODO: implement switching tracking_window by select.
  Window tracking_window = DefaultRootWindow(display);

  XRRScreenConfiguration *screen_config = XRRGetScreenInfo(display, DefaultRootWindow(display));

  short rate = XRRConfigCurrentRate(screen_config);

  printf("Screen rate: %d\n", rate);

  int screen = XDefaultScreen(display);
  
  int *maj = malloc(sizeof(int));
  int *min = malloc(sizeof(int));

	if (glXQueryVersion(display, maj, min))
	{
		if ((*maj == 1 && *min <3) || (*maj < 1))
    {
      fprintf(stderr, "Invalid GLX version. Required >=1.3\n");
    }
		printf("GLX Version: %d.%d\n", *maj, *min);
	}

	printf("%s\n", glXQueryExtensionsString(display, screen));

  free(maj);
  free(min);

  int attrs[] = 
  {
    GLX_RGBA,
    GLX_DEPTH_SIZE, 24,
    GLX_DOUBLEBUFFER,
    None
  };

  XVisualInfo *visual_info = glXChooseVisual(display, screen, attrs);
  if (visual_info == NULL)
  {
    fprintf(stderr, "No apporpriate visual found");
    exit(1);
  }

  printf("Visual %lu selected\n", visual_info->visualid);

  printf("Initializing XSetWindowAttributes.\n");
  XSetWindowAttributes *set_window_attributes = malloc(sizeof(XSetWindowAttributes));

  set_window_attributes->colormap = XCreateColormap(display, DefaultRootWindow(display), visual_info->visual, AllocNone);

  set_window_attributes->event_mask = event_mask;

  if (!windowed)
  {
    set_window_attributes->override_redirect = 1;
    set_window_attributes->save_under = 1;
  }

  printf("Initializing XWindowAttributes.\n");
  XWindowAttributes *window_attributes = calloc(1, sizeof(XWindowAttributes));
  XGetWindowAttributes(display, DefaultRootWindow(display), window_attributes);

  printf("Creating Window.\n");
  Window window = XCreateWindow(display, DefaultRootWindow(display),
                                 0, 0, 
                                 window_attributes->width, window_attributes->height, 0,
                                 visual_info->depth, InputOutput, visual_info->visual,
                                 CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder,
                                 set_window_attributes);
  
  printf("Creating new X11_screenshot.\n");
  X11_screenshot *X11_screenshot = X11_screenshot_new(display, tracking_window);
  printf("Done creating new X11_screenshot.\n");

  printf("Mapping Window.\n");
  XMapWindow(display, window);

  printf("Creating XClassHint and setting it.\n");
  XClassHint hints = {.res_name = name, .res_class = c_class};

  XStoreName(display, window, name);
  XSetClassHint(display, window, &hints);

  printf("Creating Atom delete_message.\n");
  Atom delete_message = XInternAtom(display, "WM_DELETE_WINDOW", 0);

  printf("Setting WM protocols.\n");
  XSetWMProtocols(display, window, &delete_message, 1);


  printf("Reading shader files.\n");
  Shader *vertex_shader = shader_read("src/shaders/vert.glsl");
  Shader *fragment_shader = shader_read("src/shaders/frag.glsl");
  printf("Done reading shader files.\n");

  int w = X11_screenshot->image->width;
  int h = X11_screenshot->image->height;
	
  app->config = config;
  app->vert = vertex_shader;
  app->frag = fragment_shader;
  app->display = display;
  app->root_window = root_window;
  app->window = window;
  app->tracking_window = tracking_window;
  app->screenshot = X11_screenshot;
  app->delete_message = delete_message;
  app->width = w;
  app->height = h;
	app->rate = rate;
	app->screen = screen;

  app->gl_backend = XApp_gl_init(w, h, display, app->window, visual_info, app->vert, app->frag, app->screenshot);

  return app;
}

GL_Backend *XApp_gl_init(int w, int h, Display *display, Window window, XVisualInfo *visual_info, Shader *vertex_shader, Shader *fragment_shader, X11_screenshot *X11_screenshot)
{
  GL_Backend *gl_backend = calloc(1, sizeof(GL_Backend));

  printf("Creating GLX Context.\n");
	GLXContext context = glXCreateContext(display, visual_info, NULL, GL_TRUE);
  glXMakeCurrent(display, window, context);

  printf("Creating new shader_program.\n");
  GLuint shader_program = shader_new_program(vertex_shader, fragment_shader);

  printf("Creating vao, vbo, ebo.\n");
  GLuint vao, vbo, ebo;

  printf("Creating vertices_foo.\n");
  GLfloat vertices_foo[4][5] = 
  {
    {w, 0, 0.0, 1.0, 1.0},
    {w, h, 0.0, 1.0, 0.0},
    {0, h, 0.0, 0.0, 0.0},
    {0, 0, 0.0, 0.0, 1.0}
  };

  printf("Creating indices.\n");
  GLuint indices[6] = 
  {
    0, 1, 3,
    1, 2, 3
  };

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_foo), vertices_foo, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  GLsizei stride = (GLsizei)(5 * sizeof(GLfloat));

  GLint aPos_location = glGetAttribLocation(shader_program, "aPos");
  GLint aTexCoord_location = glGetAttribLocation(shader_program, "aTexCoord");

  glVertexAttribPointer(aPos_location, 2, GL_FLOAT, false, stride, (pointer)(0));
  glEnableVertexAttribArray(aPos_location);

  glVertexAttribPointer(aTexCoord_location, 2, GL_FLOAT, false, stride, (pointer)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(aTexCoord_location);

  GLuint texture = (GLuint)0;
  
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
               X11_screenshot->image->width, X11_screenshot->image->height, 
               0, GL_BGRA, GL_UNSIGNED_BYTE, X11_screenshot->image->data);

  glGenerateMipmap(GL_TEXTURE_2D);

  glEnable(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  gl_backend->vao            = vao;
  gl_backend->vbo            = vbo;
  gl_backend->ebo            = ebo;
  gl_backend->texture        = texture;
	gl_backend->shader_program = shader_program;

  return gl_backend;
}

void gl_delete_buffers(GL_Backend *backend)
{
	glDeleteVertexArrays(1, &backend->vao);
  glDeleteBuffers(1, &backend->vbo);
  glDeleteBuffers(1, &backend->ebo);
}

void XApp_destroy(XApp *app)
{
  free(app->vert);
  free(app->frag);
  X11_screenshot_destroy(app->screenshot, app->display);
	gl_delete_buffers(app->gl_backend);
  XDestroyWindow(app->display, app->window);
  XCloseDisplay(app->display);
  free((void *)app);
}


void XApp_draw(XImage image, Camera camera, Mouse mouse, Flashlight flashlight, GLuint shader_program, GLuint vao, GLuint texture, vec2f window_size, bool mirror)
{
  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(shader_program);


#ifdef X11_DEBUG 
  printf("shader_program: %d\n", shader_program);
  printf("camera Position: x = %f, y = %f\n", camera.position.x, camera.position.y);
  printf("window_size: x = %f, y = %f\n", window_size.x, window_size.y);
  printf("X11_screenshot_size: width = %d, height = %d\n", X11_screenshot.width, X11_screenshot.height);
  printf("cursor_pos: x = %f, y = %f\n", mouse.curr.x, mouse.curr.y);

  printf("uniform location (tex): %d\n", glGetUniformLocation(shader_program, "tex"));
  printf("uniform location (cameraPos): %d\n", glGetUniformLocation(shader_program, "cameraPos"));
  printf("uniform location (cameraScale): %d\n", glGetUniformLocation(shader_program, "cameraScale"));
  printf("uniform location (X11_screenshotSize): %d\n", glGetUniformLocation(shader_program, "X11_screenshotSize"));
  printf("uniform location (windowSize): %d\n", glGetUniformLocation(shader_program, "windowSize"));
  printf("uniform location (cursorPos): %d\n", glGetUniformLocation(shader_program, "cursorPos"));
  printf("uniform location (flShadow): %d\n", glGetUniformLocation(shader_program, "flShadow"));
  printf("uniform location (flRadius): %d\n", glGetUniformLocation(shader_program, "flRadius"));
#endif

  glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);
  glUniform2f(glGetUniformLocation(shader_program, "cameraPos"), camera.position.x, camera.position.y);
  glUniform1f(glGetUniformLocation(shader_program, "cameraScale"), camera.scale);
  glUniform2f(glGetUniformLocation(shader_program, "X11_screenshotSize"), image.width, image.height);
  glUniform2f(glGetUniformLocation(shader_program, "windowSize"), window_size.x, window_size.y);
  glUniform2f(glGetUniformLocation(shader_program, "cursorPos"), mouse.curr.x, mouse.curr.y);
  glUniform1f(glGetUniformLocation(shader_program, "flShadow"), flashlight.shadow);
  glUniform1f(glGetUniformLocation(shader_program, "flRadius"), flashlight.radius);
  glUniform1f(glGetUniformLocation(shader_program, "mirror"), mirror ? 1 : 0);

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}


int XApp_error_handler(Display *display, XErrorEvent *error_event)
{
  char text[256], request[64], num[16];

  XGetErrorText(display, error_event->error_code, text, sizeof(text));
  snprintf(num, sizeof(num), "%d", error_event->error_code);
  XGetErrorDatabaseText(display, "XRequest", num, "unknown", request, sizeof(request));
 fprintf(stderr, "X11 ERROR: %s | request=%s (opcode %d, minor %d) | resource=0x%lx | serial=%lu\n",
          text, request, error_event->request_code, error_event->minor_code, error_event->resourceid, error_event->serial);
  return 0;
}

vec2f get_cursor_position(Display *display)
{
  Window *root = malloc(sizeof(Window)), *child = malloc(sizeof(Window));
  int root_x, root_y, win_x, win_y;
  unsigned int mask;

  XQueryPointer(display, DefaultRootWindow(display), 
                root, child, &root_x, &root_y, &win_x, &win_y, &mask);

  return vec2((float)root_x, (float)root_y);
}

Window select_window(Display *display)
{
  Cursor cursor = XCreateFontCursor(display, XC_crosshair);
  XFreeCursor(display, cursor);

  Window root = DefaultRootWindow(display);
  XGrabPointer(display, root, 0, ButtonMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);
  XUngrabPointer(display, CurrentTime);

  XGrabKeyboard(display, root, 0, GrabModeAsync, GrabModeAsync, CurrentTime);
  XUngrabKeyboard(display, CurrentTime);

  XEvent *event = NULL;

  while (true)
  {
    XNextEvent(display, event);

    switch (event->type)
    {
      case ButtonPress:
        return event->xbutton.subwindow;
      case KeyPress:
        return root;
      default:
        return 0L;
    }
  }

  return root;
}

// omg variadic arguments in C is so annoying...
// TODO: implement A NORMAL log function with variadic arguments
void XApp_log(char *name)
{
}
