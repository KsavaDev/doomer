#include "navigation.h"
#include "config.h"
#include "../util/la.h"
#include <X11/Xlib.h>
#include "math.h"
#include <stdio.h>

const float FL_DELTA_RADIUS_DECELERATION_C = FL_DELTA_RADIUS_DECELERATION;

void flashlight_update(Flashlight *flashlight, float dt)
{
  if (fabs(flashlight->delta_radius) > 1.0)
  {
    flashlight->radius = fmax(0.0, flashlight->radius + flashlight->delta_radius * dt);
    flashlight->delta_radius -= flashlight->delta_radius * FL_DELTA_RADIUS_DECELERATION_C * dt;
  }
  if (flashlight->is_enabled)
    flashlight->shadow = fmin(flashlight->shadow + 6.0 * dt, 0.8);
  else 
  {
    flashlight->shadow = fmax(flashlight->shadow - 6.0 * dt, 0.0);
  }
}

vec2f world(Camera camera, vec2f v)
{
	return vec2f_div_scalar(v, camera.scale);
}

void camera_update(Camera *camera, Config config, float dt, Mouse mouse, vec2f window_size)
{
	if (fabs(camera->delta_scale) > 0.5)
	{
		// p0, p1 = (camera->scale_pivot - (window_size * 0.5)) / camera->scale;
		vec2f p0 = vec2f_div_scalar(vec2f_sub(camera->scale_pivot, vec2f_mul_scalar(window_size, 0.5)), camera->scale);
		camera->scale = fmax(camera->scale + camera->delta_scale * dt, config.min_scale);

		vec2f p1 = vec2f_div_scalar(vec2f_sub(camera->scale_pivot, vec2f_mul_scalar(window_size, 0.5)), camera->scale);
		vec2f_add_assign(&camera->position, vec2f_sub(p0, p1));

		camera->delta_scale -= camera->delta_scale * dt * config.scale_friction;

		if (!(mouse.drag) && (vec2f_length(camera->velocity) > VELOCITY_THRESHOLD))
		{
			vec2f_add_assign(&camera->position, vec2f_mul_scalar(camera->velocity, dt));
			vec2f_sub_assign(&camera->velocity, vec2f_mul_scalar(camera->velocity, dt * config.drag_friction));
		}
	}
}
