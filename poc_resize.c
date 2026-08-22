/*
 * Cage: A Wayland kiosk.
 *
 * Temporary characterization of the original Linguum POC divider grab.
 * Generic ResizeBoundary replaces this file in M1-WP05.
 *
 * See the LICENSE file accompanying this file.
 */

#include "poc_resize.h"

#define POC_RESIZE_HIT_SLOP 10
#define POC_BROWSER_MIN_WIDTH 360
#define POC_BROWSER_MAX_WIDTH 1200

bool
cg_poc_resize_hit_test(struct cg_poc_rect output, int browser_width, double x, double y)
{
	double divider_x;
	double delta;

	if (browser_width <= 0) {
		return false;
	}

	divider_x = output.x + output.width - browser_width;
	delta = x - divider_x;
	return y >= output.y && y < output.y + output.height && delta >= -POC_RESIZE_HIT_SLOP &&
	       delta <= POC_RESIZE_HIT_SLOP;
}

bool
cg_poc_resize_begin(struct cg_poc_resize_state *state, struct cg_poc_rect output, int browser_width, double x, double y)
{
	if (!state || state->active || !cg_poc_resize_hit_test(output, browser_width, x, y)) {
		return false;
	}

	state->active = true;
	state->start_x = x;
	state->start_width = browser_width;
	return true;
}

bool
cg_poc_resize_update(struct cg_poc_resize_state *state, double x, int *width_out)
{
	double delta;
	int width;

	if (!state || !state->active || !width_out) {
		return false;
	}

	delta = x - state->start_x;
	width = state->start_width - (int) delta;
	if (width < POC_BROWSER_MIN_WIDTH) {
		width = POC_BROWSER_MIN_WIDTH;
	} else if (width > POC_BROWSER_MAX_WIDTH) {
		width = POC_BROWSER_MAX_WIDTH;
	}

	*width_out = width;
	return true;
}

bool
cg_poc_resize_end(struct cg_poc_resize_state *state)
{
	if (!state || !state->active) {
		return false;
	}

	state->active = false;
	return true;
}
