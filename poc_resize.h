#ifndef CG_POC_RESIZE_H
#define CG_POC_RESIZE_H

#include <stdbool.h>

#include "poc_layout.h"

struct cg_poc_resize_state {
	bool active;
	double start_x;
	int start_width;
};

bool cg_poc_resize_hit_test(struct cg_poc_rect output, int browser_width, double x, double y);
bool cg_poc_resize_begin(struct cg_poc_resize_state *state, struct cg_poc_rect output, int browser_width, double x,
			 double y);
bool cg_poc_resize_update(struct cg_poc_resize_state *state, double x, int *width_out);
bool cg_poc_resize_end(struct cg_poc_resize_state *state);

#endif
