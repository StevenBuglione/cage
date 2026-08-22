#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "poc_layout.h"

static void
assert_rect(struct cg_poc_rect actual, int x, int y, int width, int height)
{
	assert(actual.x == x);
	assert(actual.y == y);
	assert(actual.width == width);
	assert(actual.height == height);
}

static void
test_width_parser(void)
{
	int width = 777;

	assert(cg_poc_layout_parse_width("360", &width));
	assert(width == 360);
	assert(cg_poc_layout_parse_width("1200", &width));
	assert(width == 1200);
	assert(cg_poc_layout_parse_width(" +620", &width));
	assert(width == 620);

	assert(!cg_poc_layout_parse_width(NULL, &width));
	assert(!cg_poc_layout_parse_width("", &width));
	assert(!cg_poc_layout_parse_width("359", &width));
	assert(!cg_poc_layout_parse_width("1201", &width));
	assert(!cg_poc_layout_parse_width("620px", &width));
	assert(!cg_poc_layout_parse_width("620", NULL));
}

static void
test_title_classification(void)
{
	assert(cg_poc_layout_classify_title(false, "Linguum Workspace") == CG_POC_SURFACE_DEFAULT);
	assert(cg_poc_layout_classify_title(true, "Linguum Workspace") == CG_POC_SURFACE_WORKSPACE);
	assert(cg_poc_layout_classify_title(true, "Linguum Workspace — project") == CG_POC_SURFACE_WORKSPACE);
	assert(cg_poc_layout_classify_title(true, "Linguum Browser Controls") == CG_POC_SURFACE_CONTROLS);
	assert(cg_poc_layout_classify_title(true, "Linguum Browser Controls — active") == CG_POC_SURFACE_CONTROLS);
	assert(cg_poc_layout_classify_title(true, "Firefox") == CG_POC_SURFACE_BROWSER);
	assert(cg_poc_layout_classify_title(true, NULL) == CG_POC_SURFACE_BROWSER);
}

static void
test_three_surface_rectangles(void)
{
	const struct cg_poc_rect output = {.x = 13, .y = 17, .width = 1440, .height = 900};
	struct cg_poc_rect target;

	assert(cg_poc_layout_rect(output, 620, CG_POC_SURFACE_WORKSPACE, &target));
	assert_rect(target, 13, 17, 820, 900);
	assert(cg_poc_layout_rect(output, 620, CG_POC_SURFACE_CONTROLS, &target));
	assert_rect(target, 833, 17, 620, 40);
	assert(cg_poc_layout_rect(output, 620, CG_POC_SURFACE_BROWSER, &target));
	assert_rect(target, 833, 57, 620, 860);
}

static void
test_narrow_output_fallback(void)
{
	const struct cg_poc_rect output = {.x = 0, .y = 0, .width = 1000, .height = 700};
	struct cg_poc_rect target;

	assert(cg_poc_layout_rect(output, 700, CG_POC_SURFACE_WORKSPACE, &target));
	assert_rect(target, 0, 0, 500, 700);
	assert(cg_poc_layout_rect(output, 700, CG_POC_SURFACE_CONTROLS, &target));
	assert_rect(target, 500, 0, 500, 40);
	assert(cg_poc_layout_rect(output, 700, CG_POC_SURFACE_BROWSER, &target));
	assert_rect(target, 500, 40, 500, 660);
}

static void
test_minimum_target_dimensions(void)
{
	const struct cg_poc_rect output = {.x = 4, .y = 8, .width = 1, .height = 10};
	struct cg_poc_rect target;

	assert(cg_poc_layout_rect(output, 360, CG_POC_SURFACE_CONTROLS, &target));
	assert_rect(target, 5, 8, 1, 40);
	assert(cg_poc_layout_rect(output, 360, CG_POC_SURFACE_BROWSER, &target));
	assert_rect(target, 5, 48, 1, 1);
}

static void
test_inactive_layout(void)
{
	const struct cg_poc_rect output = {.x = 0, .y = 0, .width = 1440, .height = 900};
	struct cg_poc_rect target;

	assert(!cg_poc_layout_rect(output, 0, CG_POC_SURFACE_BROWSER, &target));
	assert(!cg_poc_layout_rect(output, 620, CG_POC_SURFACE_DEFAULT, &target));
	assert(!cg_poc_layout_rect(output, 620, (enum cg_poc_surface_role) 99, &target));
	assert(!cg_poc_layout_rect(output, 620, CG_POC_SURFACE_BROWSER, NULL));
}

int
main(void)
{
	test_width_parser();
	test_title_classification();
	test_three_surface_rectangles();
	test_narrow_output_fallback();
	test_minimum_target_dimensions();
	test_inactive_layout();
	return 0;
}
