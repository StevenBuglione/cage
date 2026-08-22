/*
 * Cage: A Wayland kiosk.
 *
 * Temporary characterization of the Linguum POC layout patch. This file is
 * removed when the explicit surface registry and generic scene model land.
 *
 * See the LICENSE file accompanying this file.
 */

#include <stdlib.h>
#include <string.h>

#include "poc_layout.h"

#define POC_CONTROLS_HEIGHT 40
#define POC_BROWSER_MIN_WIDTH 360
#define POC_BROWSER_MAX_WIDTH 1200
#define POC_WORKSPACE_MIN_WIDTH 400
#define POC_LAYOUT_MESSAGE_CAPACITY 32

bool
cg_poc_layout_parse_width(const char *value, int *width_out)
{
	char *end = NULL;
	long width;

	if (!value || !*value || !width_out) {
		return false;
	}

	width = strtol(value, &end, 10);
	if (*end != '\0' || width < POC_BROWSER_MIN_WIDTH || width > POC_BROWSER_MAX_WIDTH) {
		return false;
	}

	*width_out = (int) width;
	return true;
}

bool
cg_poc_layout_parse_message(const char *message, size_t size, int *width_out)
{
	char value[POC_LAYOUT_MESSAGE_CAPACITY];

	if (!message || !width_out || size == 0 || size >= sizeof(value) || memchr(message, '\0', size)) {
		return false;
	}

	memcpy(value, message, size);
	value[size] = '\0';
	return cg_poc_layout_parse_width(value, width_out);
}

bool
cg_poc_layout_socket_path_valid(const char *path, const char *runtime_dir, size_t path_capacity)
{
	size_t runtime_length;

	if (!path || !*path || !runtime_dir || !*runtime_dir || path_capacity == 0) {
		return false;
	}

	runtime_length = strlen(runtime_dir);
	if (runtime_dir[0] != '/' || runtime_dir[runtime_length - 1] == '/' || path[0] != '/' ||
	    strlen(path) >= path_capacity) {
		return false;
	}

	return strncmp(path, runtime_dir, runtime_length) == 0 && path[runtime_length] == '/' &&
	       path[runtime_length + 1] != '\0';
}

bool
cg_poc_layout_rect(struct cg_poc_rect output, int browser_width, enum cg_poc_surface_role role,
		   struct cg_poc_rect *target_out)
{
	struct cg_poc_rect target = output;
	int workspace_width;

	if (!target_out || browser_width <= 0 ||
	    (role != CG_POC_SURFACE_WORKSPACE && role != CG_POC_SURFACE_CONTROLS && role != CG_POC_SURFACE_BROWSER)) {
		return false;
	}

	if (browser_width > output.width - POC_WORKSPACE_MIN_WIDTH) {
		browser_width = output.width / 2;
	}
	workspace_width = output.width - browser_width;

	switch (role) {
	case CG_POC_SURFACE_WORKSPACE:
		target.width = workspace_width;
		break;
	case CG_POC_SURFACE_CONTROLS:
		target.x += workspace_width;
		target.width = browser_width;
		target.height = POC_CONTROLS_HEIGHT;
		break;
	case CG_POC_SURFACE_BROWSER:
		target.x += workspace_width;
		target.y += POC_CONTROLS_HEIGHT;
		target.width = browser_width;
		target.height -= POC_CONTROLS_HEIGHT;
		break;
	case CG_POC_SURFACE_DEFAULT:
		return false;
	}

	if (target.width < 1) {
		target.width = 1;
	}
	if (target.height < 1) {
		target.height = 1;
	}

	*target_out = target;
	return true;
}
