#include <assert.h>
#include <string.h>

#include "scene_model.h"

static struct cg_surface_token
token_for(cg_surface_id surface_id)
{
	struct cg_surface_token token = {0};
	token.bytes[0] = (uint8_t) surface_id;
	token.bytes[8] = (uint8_t) (surface_id >> 8);
	token.bytes[CG_SURFACE_TOKEN_SIZE - 1] = 0xa5;
	return token;
}

static void
register_surface(struct cg_surface_registry *registry, cg_surface_id surface_id, enum cg_surface_kind kind,
		 bool has_parent, cg_surface_id parent_surface_id)
{
	struct cg_surface_registration_request request = {
		.scene_id = 7,
		.surface_id = surface_id,
		.token = token_for(surface_id),
		.kind = kind,
		.has_parent = has_parent,
		.parent_surface_id = parent_surface_id,
		.association_timeout_ms = 5000,
	};
	assert(cg_surface_registry_register(registry, &request, 0) == CG_SURFACE_REGISTRY_OK);
}

static void
associate_surface(struct cg_surface_registry *registry, cg_surface_id surface_id, uintptr_t opaque_surface)
{
	struct cg_surface_token token = token_for(surface_id);
	struct cg_surface_identity identity;
	assert(cg_surface_registry_associate(registry, &token, opaque_surface, 1, &identity) == CG_SURFACE_REGISTRY_OK);
}

static struct cg_scene_surface_state
surface(cg_surface_id surface_id, int32_t x, int32_t y, int32_t width, int32_t height, int32_t z_index)
{
	return (struct cg_scene_surface_state) {
		.surface_id = surface_id,
		.bounds = {.x = x, .y = y, .width = width, .height = height},
		.z_index = z_index,
		.visible = true,
		.accepts_input = true,
	};
}

static struct cg_scene_snapshot
base_snapshot(void)
{
	struct cg_scene_snapshot snapshot = {
		.scene_id = 7,
		.output_id = 11,
		.revision = 1,
		.snapshot_output_width = 1000,
		.snapshot_output_height = 700,
		.surface_count = 2,
		.has_focused_surface = true,
		.focused_surface_id = 200,
	};
	snapshot.surfaces[0] = surface(100, 0, 0, 1000, 700, 0);
	snapshot.surfaces[1] = surface(200, 600, 40, 400, 660, 10);
	return snapshot;
}

static void
setup(struct cg_scene_model *model, struct cg_surface_registry *registry)
{
	cg_scene_model_init(model);
	cg_surface_registry_init(registry);
	assert(cg_scene_model_create(model, 7, 11, 1000, 700) == CG_SCENE_OK);
	register_surface(registry, 100, CG_SURFACE_KIND_APP_VIEW, false, 0);
	register_surface(registry, 200, CG_SURFACE_KIND_FIREFOX_VIEW, false, 0);
}

static void
test_create_destroy_and_capacity(void)
{
	struct cg_scene_model model;

	cg_scene_model_init(&model);
	assert(cg_scene_model_create(NULL, 1, 1, 100, 100) == CG_SCENE_INVALID);
	assert(cg_scene_model_create(&model, 0, 1, 100, 100) == CG_SCENE_INVALID);
	assert(cg_scene_model_create(&model, 1, 0, 100, 100) == CG_SCENE_INVALID);
	assert(cg_scene_model_create(&model, 1, 1, 0, 100) == CG_SCENE_INVALID);
	for (size_t index = 0; index < CG_SCENE_CAPACITY; index++) {
		assert(cg_scene_model_create(&model, index + 1, index + 11, 100, 100) == CG_SCENE_OK);
	}
	assert(cg_scene_model_create(&model, 99, 99, 100, 100) == CG_SCENE_CAPACITY_EXCEEDED);
	assert(cg_scene_model_create(&model, 1, 11, 100, 100) == CG_SCENE_OK);
	assert(cg_scene_model_create(&model, 1, 12, 100, 100) == CG_SCENE_DUPLICATE_ID);
	assert(cg_scene_model_destroy(&model, 1) == CG_SCENE_OK);
	assert(cg_scene_model_destroy(&model, 1) == CG_SCENE_NOT_FOUND);
	assert(model.scene_count == CG_SCENE_CAPACITY - 1);
}

static void
test_revision_atomicity_and_idempotence(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_scene_snapshot snapshot = base_snapshot();
	struct cg_scene_snapshot conflict;
	const struct cg_scene_record *record;

	setup(&model, &registry);
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(model.applied_snapshots == 1);
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(model.applied_snapshots == 1);
	struct cg_scene_surface_state reordered = snapshot.surfaces[0];
	snapshot.surfaces[0] = snapshot.surfaces[1];
	snapshot.surfaces[1] = reordered;
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(model.applied_snapshots == 1);
	snapshot = base_snapshot();

	conflict = snapshot;
	conflict.surfaces[1].bounds.x = 500;
	assert(cg_scene_model_apply(&model, &registry, &conflict) == CG_SCENE_REVISION_CONFLICT);
	conflict.revision = 0;
	assert(cg_scene_model_apply(&model, &registry, &conflict) == CG_SCENE_REVISION_STALE);
	record = cg_scene_model_find(&model, 7);
	assert(record && record->snapshot.revision == 1);
	assert(record->snapshot.surfaces[1].bounds.x == 600);
	assert(model.rejected_snapshots == 2);
}

static void
test_add_update_remove_and_validation(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_scene_snapshot first = base_snapshot();
	struct cg_scene_snapshot next;

	setup(&model, &registry);
	assert(cg_scene_model_apply(&model, &registry, &first) == CG_SCENE_OK);
	register_surface(&registry, 300, CG_SURFACE_KIND_OVERLAY, false, 0);
	next = first;
	next.revision = 2;
	next.surface_count = 2;
	next.surfaces[0].bounds.width = 550;
	next.surfaces[1] = surface(300, 100, 100, 200, 120, 20);
	next.focused_surface_id = 300;
	assert(cg_scene_model_apply(&model, &registry, &next) == CG_SCENE_OK);
	assert(!cg_scene_snapshot_find_surface(&cg_scene_model_find(&model, 7)->snapshot, 200));
	assert(cg_scene_snapshot_find_surface(&cg_scene_model_find(&model, 7)->snapshot, 300));

	struct cg_scene_snapshot invalid = next;
	invalid.revision = 3;
	invalid.surfaces[1].surface_id = 999;
	assert(cg_scene_model_apply(&model, &registry, &invalid) == CG_SCENE_SURFACE_NOT_REGISTERED);
	assert(cg_scene_model_find(&model, 7)->snapshot.revision == 2);
	invalid = next;
	invalid.revision = 3;
	invalid.surfaces[1].surface_id = 100;
	assert(cg_scene_model_apply(&model, &registry, &invalid) == CG_SCENE_DUPLICATE_ID);
	invalid = next;
	invalid.revision = 3;
	invalid.surfaces[1].visible = false;
	assert(cg_scene_model_apply(&model, &registry, &invalid) == CG_SCENE_INVALID);
	invalid = next;
	invalid.revision = 3;
	invalid.focused_surface_id = 200;
	assert(cg_scene_model_apply(&model, &registry, &invalid) == CG_SCENE_FOCUS_INVALID);
}

static void
test_parent_and_boundary_validation(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_scene_snapshot snapshot = base_snapshot();

	setup(&model, &registry);
	register_surface(&registry, 201, CG_SURFACE_KIND_POPUP, true, 200);
	snapshot.surface_count = 3;
	snapshot.surfaces[2] = surface(201, 700, 80, 200, 200, 30);
	snapshot.surfaces[2].has_parent = true;
	snapshot.surfaces[2].parent_surface_id = 200;
	snapshot.resize_boundary_count = 1;
	snapshot.resize_boundaries[0] = (struct cg_scene_resize_boundary) {
		.boundary_id = 55,
		.target_surface_id = 200,
		.edge = CG_SCENE_RESIZE_EDGE_LEFT,
		.minimum_size = 360,
		.maximum_size = 900,
		.hit_slop = 10,
		.cursor = CG_SCENE_RESIZE_CURSOR_COLUMN,
		.enabled = true,
		.visible = true,
	};
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);

	struct cg_scene_snapshot invalid = snapshot;
	invalid.revision = 2;
	invalid.surfaces[2].parent_surface_id = 100;
	assert(cg_scene_model_apply(&model, &registry, &invalid) == CG_SCENE_SURFACE_PARENT_MISMATCH);
	invalid = snapshot;
	invalid.revision = 2;
	invalid.resize_boundaries[0].target_surface_id = 999;
	assert(cg_scene_model_apply(&model, &registry, &invalid) == CG_SCENE_BOUNDARY_INVALID);
	invalid = snapshot;
	invalid.revision = 2;
	invalid.resize_boundary_count = 2;
	invalid.resize_boundaries[1] = invalid.resize_boundaries[0];
	invalid.resize_boundaries[0].target_surface_id = 200;
	invalid.resize_boundaries[1].target_surface_id = 200;
	assert(cg_scene_model_apply(&model, &registry, &invalid) == CG_SCENE_DUPLICATE_ID);
}

static void
test_clip_z_order_visibility_and_association(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_scene_snapshot snapshot = base_snapshot();
	const struct cg_scene_surface_state *hit;

	setup(&model, &registry);
	snapshot.surfaces[1].bounds = (struct cg_scene_rect) {.x = 400, .y = 0, .width = 600, .height = 700};
	snapshot.surfaces[1].has_clip = true;
	snapshot.surfaces[1].clip = (struct cg_scene_rect) {.x = 500, .y = 100, .width = 300, .height = 300};
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(!cg_scene_model_hit_test(&model, &registry, 7, 600, 200));
	associate_surface(&registry, 100, 0x1000);
	hit = cg_scene_model_hit_test(&model, &registry, 7, 600, 200);
	assert(hit && hit->surface_id == 100);
	associate_surface(&registry, 200, 0x2000);
	hit = cg_scene_model_hit_test(&model, &registry, 7, 600, 200);
	assert(hit && hit->surface_id == 200);
	hit = cg_scene_model_hit_test(&model, &registry, 7, 450, 50);
	assert(hit && hit->surface_id == 100);

	snapshot.revision = 2;
	snapshot.surfaces[1].visible = false;
	snapshot.surfaces[1].accepts_input = false;
	snapshot.has_focused_surface = true;
	snapshot.focused_surface_id = 100;
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	hit = cg_scene_model_hit_test(&model, &registry, 7, 600, 200);
	assert(hit && hit->surface_id == 100);
}

static void
test_output_resize_and_reconnect_snapshot(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_scene_snapshot snapshot = base_snapshot();
	struct cg_scene_rect resolved;

	setup(&model, &registry);
	snapshot.surfaces[1].bounds = (struct cg_scene_rect) {.x = 900, .y = 0, .width = 400, .height = 700};
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(cg_scene_model_resolve_surface(&model, 7, 200, &resolved));
	assert(resolved.x == 900 && resolved.width == 100);
	assert(cg_scene_model_resize_output(&model, 7, 1200, 800) == CG_SCENE_OK);
	assert(cg_scene_model_resolve_surface(&model, 7, 200, &resolved));
	assert(resolved.width == 300 && resolved.height == 700);
	assert(cg_scene_model_resize_output(&model, 7, 800, 800) == CG_SCENE_OK);
	assert(!cg_scene_model_resolve_surface(&model, 7, 200, &resolved));

	cg_scene_model_reset(&model);
	cg_surface_registry_reset(&registry);
	assert(cg_scene_model_create(&model, 7, 11, 1200, 800) == CG_SCENE_OK);
	register_surface(&registry, 100, CG_SURFACE_KIND_APP_VIEW, false, 0);
	register_surface(&registry, 200, CG_SURFACE_KIND_FIREFOX_VIEW, false, 0);
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(cg_scene_model_find(&model, 7)->snapshot.revision == 1);
}

static void
test_output_anchors_project_without_mutating_revision(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_scene_snapshot snapshot = base_snapshot();
	struct cg_scene_surface_state layout;
	const struct cg_scene_record *record;

	setup(&model, &registry);
	snapshot.surfaces[0].output_anchor_mask = CG_SCENE_OUTPUT_ANCHOR_MASK;
	snapshot.surfaces[1].output_anchor_mask = CG_SCENE_OUTPUT_ANCHOR_MASK;
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(cg_scene_model_resize_output(&model, 7, 1200, 800) == CG_SCENE_OK);
	assert(cg_scene_model_layout_surface(&model, 7, 100, &layout));
	assert(layout.bounds.x == 0 && layout.bounds.y == 0);
	assert(layout.bounds.width == 1200 && layout.bounds.height == 800);
	assert(cg_scene_model_layout_surface(&model, 7, 200, &layout));
	assert(layout.bounds.x == 600 && layout.bounds.y == 40);
	assert(layout.bounds.width == 600 && layout.bounds.height == 760);
	record = cg_scene_model_find(&model, 7);
	assert(record && record->snapshot.revision == 1);
	assert(record->snapshot.surfaces[0].bounds.width == 1000);
	assert(record->snapshot.surfaces[1].bounds.width == 400);

	snapshot.revision = 2;
	snapshot.surfaces[0].output_anchor_mask = 0x10;
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_INVALID);
}

static void
test_output_anchors_use_the_transaction_basis(void)
{
	struct cg_scene_model model;
	struct cg_surface_registry registry;
	struct cg_scene_snapshot snapshot = base_snapshot();
	struct cg_scene_surface_state layout;

	setup(&model, &registry);
	snapshot.surfaces[0].output_anchor_mask = CG_SCENE_OUTPUT_ANCHOR_MASK;
	assert(cg_scene_model_resize_output(&model, 7, 1920, 1080) == CG_SCENE_OK);
	assert(cg_scene_model_apply(&model, &registry, &snapshot) == CG_SCENE_OK);
	assert(cg_scene_model_resize_output(&model, 7, 2560, 1440) == CG_SCENE_OK);
	assert(cg_scene_model_layout_surface(&model, 7, 100, &layout));
	assert(layout.bounds.width == 2560 && layout.bounds.height == 1440);
}

int
main(void)
{
	test_create_destroy_and_capacity();
	test_revision_atomicity_and_idempotence();
	test_add_update_remove_and_validation();
	test_parent_and_boundary_validation();
	test_clip_z_order_visibility_and_association();
	test_output_resize_and_reconnect_snapshot();
	test_output_anchors_project_without_mutating_revision();
	test_output_anchors_use_the_transaction_basis();
	return 0;
}
