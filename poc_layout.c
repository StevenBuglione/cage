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

#define POC_WORKSPACE_TITLE "Linguum Workspace"
#define POC_CONTROLS_TITLE "Linguum Browser Controls"
#define POC_CONTROLS_HEIGHT 40
#define POC_BROWSER_MIN_WIDTH 360
#define POC_BROWSER_MAX_WIDTH 1200
#define POC_WORKSPACE_MIN_WIDTH 400

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

enum cg_poc_surface_role
cg_poc_layout_classify_title(bool enabled, const char *title)
{
	if (!enabled) {
		return CG_POC_SURFACE_DEFAULT;
	}

	if (title && strncmp(title, POC_WORKSPACE_TITLE, strlen(POC_WORKSPACE_TITLE)) == 0) {
		return CG_POC_SURFACE_WORKSPACE;
	}

	if (title && strncmp(title, POC_CONTROLS_TITLE, strlen(POC_CONTROLS_TITLE)) == 0) {
		return CG_POC_SURFACE_CONTROLS;
	}

	return CG_POC_SURFACE_BROWSER;
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
