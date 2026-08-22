#include <assert.h>
#include <stdbool.h>

#include "poc_resize.h"

static void
test_hit_slop_and_output_bounds(void)
{
	const struct cg_poc_rect output = {.x = 20, .y = 30, .width = 1400, .height = 900};
	const double divider_x = 800;

	assert(!cg_poc_resize_hit_test(output, 0, divider_x, 100));
	assert(!cg_poc_resize_hit_test(output, -1, divider_x, 100));
	assert(cg_poc_resize_hit_test(output, 620, divider_x, 30));
	assert(cg_poc_resize_hit_test(output, 620, divider_x - 10, 899));
	assert(cg_poc_resize_hit_test(output, 620, divider_x + 10, 929));
	assert(!cg_poc_resize_hit_test(output, 620, divider_x - 10.01, 100));
	assert(!cg_poc_resize_hit_test(output, 620, divider_x + 10.01, 100));
	assert(!cg_poc_resize_hit_test(output, 620, divider_x, 29.99));
	assert(!cg_poc_resize_hit_test(output, 620, divider_x, 930));
}

static void
test_grab_motion_and_release(void)
{
	const struct cg_poc_rect output = {.x = 0, .y = 0, .width = 1440, .height = 900};
	struct cg_poc_resize_state state = {0};
	int width = 0;

	assert(!cg_poc_resize_begin(&state, output, 620, 700, 100));
	assert(!state.active);
	assert(cg_poc_resize_begin(&state, output, 620, 820, 100));
	assert(state.active);
	assert(state.start_x == 820);
	assert(state.start_width == 620);
	assert(!cg_poc_resize_begin(&state, output, 620, 820, 100));

	assert(cg_poc_resize_update(&state, 720, &width));
	assert(width == 720);
	assert(cg_poc_resize_update(&state, 920, &width));
	assert(width == 520);
	assert(cg_poc_resize_update(&state, 2000, &width));
	assert(width == 360);
	assert(cg_poc_resize_update(&state, -2000, &width));
	assert(width == 1200);
	assert(cg_poc_resize_update(&state, 820.9, &width));
	assert(width == 620);

	assert(cg_poc_resize_end(&state));
	assert(!state.active);
	assert(!cg_poc_resize_update(&state, 720, &width));
	assert(!cg_poc_resize_end(&state));
}

static void
test_invalid_arguments(void)
{
	const struct cg_poc_rect output = {.x = 0, .y = 0, .width = 1440, .height = 900};
	struct cg_poc_resize_state state = {0};
	int width;

	assert(!cg_poc_resize_begin(NULL, output, 620, 820, 100));
	assert(!cg_poc_resize_update(NULL, 820, &width));
	assert(!cg_poc_resize_update(&state, 820, NULL));
	assert(!cg_poc_resize_end(NULL));
}

int
main(void)
{
	test_hit_slop_and_output_bounds();
	test_grab_motion_and_release();
	test_invalid_arguments();
	return 0;
}
