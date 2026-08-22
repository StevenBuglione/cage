#include <assert.h>
#include <string.h>

#include "resize_boundary.h"

static struct cg_surface_token
token(void)
{
	struct cg_surface_token token = {0};
	token.bytes[0] = 1;
	token.bytes[CG_SURFACE_TOKEN_SIZE - 1] = 0xa5;
	return token;
}

static void
setup(struct cg_scene_model *model, struct cg_surface_registry *registry, enum cg_scene_resize_edge edge)
{
	struct cg_surface_registration_request registration = {
		.scene_id = 7,
		.surface_id = 200,
		.token = token(),
		.kind = CG_SURFACE_KIND_FIREFOX_VIEW,
		.association_timeout_ms = 5000,
	};
	struct cg_scene_snapshot snapshot = {
		.scene_id = 7,
		.output_id = 11,
		.revision = 1,
		.surface_count = 1,
		.resize_boundary_count = 1,
	};
	struct cg_surface_identity identity;

	snapshot.surfaces[0] = (struct cg_scene_surface_state) {
		.surface_id = 200,
		.bounds = {.x = 400, .y = 100, .width = 500, .height = 400},
		.z_index = 10,
		.visible = true,
		.accepts_input = true,
	};
	snapshot.resize_boundaries[0] = (struct cg_scene_resize_boundary) {
		.boundary_id = 55,
		.target_surface_id = 200,
		.edge = edge,
		.minimum_size = 200,
		.maximum_size = 800,
		.hit_slop = 10,
		.cursor = edge == CG_SCENE_RESIZE_EDGE_LEFT || edge == CG_SCENE_RESIZE_EDGE_RIGHT
				  ? CG_SCENE_RESIZE_CURSOR_COLUMN
				  : CG_SCENE_RESIZE_CURSOR_ROW,
		.enabled = true,
	};
	cg_scene_model_init(model);
	cg_surface_registry_init(registry);
	assert(cg_scene_model_create(model, 7, 11, 1000, 700) == CG_SCENE_OK);
	assert(cg_surface_registry_register(registry, &registration, 0) == CG_SURFACE_REGISTRY_OK);
	assert(cg_scene_model_apply(model, registry, &snapshot) == CG_SCENE_OK);
	assert(cg_surface_registry_associate(registry, &registration.token, 0x2000, 1, &identity) ==
	       CG_SURFACE_REGISTRY_OK);
}

static struct cg_scene_rect
bounds(const struct cg_scene_model *model)
{
	return cg_scene_model_find(model, 7)->snapshot.surfaces[0].bounds;
}

static void
test_hit_test_and_disabled_cases(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_resize_hit hit;

	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_LEFT);
	assert(cg_resize_boundary_hit_test(&model, &registry, 390, 200, &hit));
	assert(hit.scene_id == 7 && hit.boundary_id == 55 && hit.surface_id == 200);
	assert(hit.cursor == CG_SCENE_RESIZE_CURSOR_COLUMN);
	assert(cg_resize_boundary_hit_test(&model, &registry, 410, 499, &hit));
	assert(!cg_resize_boundary_hit_test(&model, &registry, 411, 200, &hit));
	assert(!cg_resize_boundary_hit_test(&model, &registry, 400, 500, &hit));

	cg_scene_model_find_mutable(&model, 7)->snapshot.resize_boundaries[0].enabled = false;
	assert(!cg_resize_boundary_hit_test(&model, &registry, 400, 200, &hit));
	cg_scene_model_find_mutable(&model, 7)->snapshot.resize_boundaries[0].enabled = true;
	cg_scene_model_find_mutable(&model, 7)->snapshot.surfaces[0].visible = false;
	assert(!cg_resize_boundary_hit_test(&model, &registry, 400, 200, &hit));

	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_LEFT);
	cg_surface_registry_retire(&registry, 7, 200);
	assert(!cg_resize_boundary_hit_test(&model, &registry, 400, 200, &hit));
}

static void
test_all_edges_and_clamping(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_resize_session session;
	struct cg_resize_event event;
	struct cg_scene_rect rect;

	cg_resize_session_init(&session);
	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_LEFT);
	assert(cg_resize_session_begin(&session, &model, &registry, 400, 200, 0));
	assert(cg_resize_session_update(&session, &model, &registry, 800, 200, 1, &event));
	rect = bounds(&model);
	assert(rect.x == 700 && rect.width == 200);
	assert(event.type == CG_RESIZE_EVENT_BOUNDS_CHANGING);
	assert(cg_resize_session_update(&session, &model, &registry, -1000, 200, 2, &event));
	rect = bounds(&model);
	assert(rect.x == 100 && rect.width == 800);

	cg_resize_session_init(&session);
	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_RIGHT);
	assert(cg_resize_session_begin(&session, &model, &registry, 900, 200, 0));
	assert(cg_resize_session_update(&session, &model, &registry, 1000, 200, 1, &event));
	assert(bounds(&model).width == 600 && bounds(&model).x == 400);

	cg_resize_session_init(&session);
	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_TOP);
	assert(cg_resize_session_begin(&session, &model, &registry, 500, 100, 0));
	assert(cg_resize_session_update(&session, &model, &registry, 500, 200, 1, &event));
	rect = bounds(&model);
	assert(rect.y == 200 && rect.height == 300);

	cg_resize_session_init(&session);
	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_BOTTOM);
	assert(cg_resize_session_begin(&session, &model, &registry, 500, 500, 0));
	assert(cg_resize_session_update(&session, &model, &registry, 500, 600, 1, &event));
	assert(bounds(&model).height == 500 && bounds(&model).y == 100);
}

static void
test_throttle_commit_and_cancel(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_resize_session session;
	struct cg_resize_event event;

	cg_resize_session_init(&session);
	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_LEFT);
	assert(cg_resize_session_begin(&session, &model, &registry, 400, 200, 100));
	for (uint64_t tick = 0; tick < 100; tick++) {
		assert(cg_resize_session_update(&session, &model, &registry, 401 + (double) tick, 200, 101 + tick,
						&event));
		if (tick == 0) {
			assert(event.type == CG_RESIZE_EVENT_BOUNDS_CHANGING);
		} else if (tick < CG_RESIZE_EVENT_INTERVAL_MS) {
			assert(event.type == CG_RESIZE_EVENT_NONE);
		}
	}
	assert(cg_resize_session_commit(&session, &model, &event));
	assert(event.type == CG_RESIZE_EVENT_BOUNDS_COMMITTED);
	assert(event.bounds.width == bounds(&model).width);
	assert(!session.active);

	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_RIGHT);
	assert(cg_resize_session_begin(&session, &model, &registry, 900, 200, 0));
	assert(cg_resize_session_update(&session, &model, &registry, 1000, 200, 1, &event));
	assert(bounds(&model).width == 600);
	assert(cg_resize_session_cancel(&session, &model, &event));
	assert(event.type == CG_RESIZE_EVENT_CANCELLED);
	assert(bounds(&model).width == 500);
}

static void
test_target_change_and_output_resize_cancel(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_resize_session session;
	struct cg_resize_event event;

	cg_resize_session_init(&session);
	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_RIGHT);
	assert(cg_resize_session_begin(&session, &model, &registry, 900, 200, 0));
	assert(cg_scene_model_resize_output(&model, 7, 1200, 800) == CG_SCENE_OK);
	assert(cg_resize_session_update(&session, &model, &registry, 950, 200, 1, &event));
	assert(event.type == CG_RESIZE_EVENT_CANCELLED);
	assert(!session.active);

	setup(&model, &registry, CG_SCENE_RESIZE_EDGE_RIGHT);
	assert(cg_resize_session_begin(&session, &model, &registry, 900, 200, 0));
	struct cg_scene_snapshot replacement = cg_scene_model_find(&model, 7)->snapshot;
	replacement.revision = 2;
	replacement.surfaces[0].visible = false;
	replacement.surfaces[0].accepts_input = false;
	assert(cg_scene_model_apply(&model, &registry, &replacement) == CG_SCENE_OK);
	assert(cg_resize_session_update(&session, &model, &registry, 950, 200, 1, &event));
	assert(event.type == CG_RESIZE_EVENT_CANCELLED);
	assert(!session.active);
	assert(cg_scene_model_find(&model, 7)->snapshot.revision == 2);
	assert(!cg_scene_model_find(&model, 7)->snapshot.surfaces[0].visible);
}

int
main(void)
{
	test_hit_test_and_disabled_cases();
	test_all_edges_and_clamping();
	test_throttle_commit_and_cancel();
	test_target_change_and_output_resize_cancel();
	return 0;
}
