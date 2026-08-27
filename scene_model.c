/*
 * Cage: A Wayland kiosk.
 *
 * Bounded generic scene snapshots and atomic revision application.
 *
 * See the LICENSE file accompanying this file.
 */

#include <limits.h>
#include <string.h>

#include "scene_model.h"

static bool
output_size_valid(uint32_t width, uint32_t height)
{
	return width > 0 && height > 0 && width <= INT32_MAX && height <= INT32_MAX;
}

static bool
rect_valid(struct cg_scene_rect rect)
{
	return rect.width > 0 && rect.height > 0;
}

static bool
rect_equal(struct cg_scene_rect left, struct cg_scene_rect right)
{
	return left.x == right.x && left.y == right.y && left.width == right.width && left.height == right.height;
}

static bool
surface_equal(const struct cg_scene_surface_state *left, const struct cg_scene_surface_state *right)
{
	return left->surface_id == right->surface_id && rect_equal(left->bounds, right->bounds) &&
	       left->has_clip == right->has_clip && (!left->has_clip || rect_equal(left->clip, right->clip)) &&
	       left->z_index == right->z_index && left->visible == right->visible &&
	       left->accepts_input == right->accepts_input && left->has_parent == right->has_parent &&
	       left->parent_surface_id == right->parent_surface_id && left->modal == right->modal &&
	       left->output_anchor_mask == right->output_anchor_mask;
}

static bool
boundary_equal(const struct cg_scene_resize_boundary *left, const struct cg_scene_resize_boundary *right)
{
	return left->boundary_id == right->boundary_id && left->target_surface_id == right->target_surface_id &&
	       left->edge == right->edge && left->minimum_size == right->minimum_size &&
	       left->maximum_size == right->maximum_size && left->hit_slop == right->hit_slop &&
	       left->cursor == right->cursor && left->enabled == right->enabled && left->visible == right->visible;
}

static bool
snapshot_equal(const struct cg_scene_snapshot *left, const struct cg_scene_snapshot *right)
{
	if (left->scene_id != right->scene_id || left->output_id != right->output_id ||
	    left->revision != right->revision || left->surface_count != right->surface_count ||
	    left->snapshot_output_width != right->snapshot_output_width ||
	    left->snapshot_output_height != right->snapshot_output_height ||
	    left->resize_boundary_count != right->resize_boundary_count ||
	    left->has_focused_surface != right->has_focused_surface ||
	    left->focused_surface_id != right->focused_surface_id) {
		return false;
	}
	for (uint16_t index = 0; index < left->surface_count; index++) {
		const struct cg_scene_surface_state *matching =
			cg_scene_snapshot_find_surface(right, left->surfaces[index].surface_id);
		if (!matching || !surface_equal(&left->surfaces[index], matching)) {
			return false;
		}
	}
	for (uint16_t index = 0; index < left->resize_boundary_count; index++) {
		const struct cg_scene_resize_boundary *matching = NULL;
		for (uint16_t other = 0; other < right->resize_boundary_count; other++) {
			if (right->resize_boundaries[other].boundary_id == left->resize_boundaries[index].boundary_id) {
				matching = &right->resize_boundaries[other];
				break;
			}
		}
		if (!matching || !boundary_equal(&left->resize_boundaries[index], matching)) {
			return false;
		}
	}
	return true;
}

void
cg_scene_model_init(struct cg_scene_model *model)
{
	if (model) {
		memset(model, 0, sizeof(*model));
	}
}

void
cg_scene_model_reset(struct cg_scene_model *model)
{
	cg_scene_model_init(model);
}

const struct cg_scene_record *
cg_scene_model_find(const struct cg_scene_model *model, cg_scene_id scene_id)
{
	if (!model || scene_id == 0) {
		return NULL;
	}
	for (size_t index = 0; index < CG_SCENE_CAPACITY; index++) {
		if (model->scenes[index].occupied && model->scenes[index].snapshot.scene_id == scene_id) {
			return &model->scenes[index];
		}
	}
	return NULL;
}

struct cg_scene_record *
cg_scene_model_find_mutable(struct cg_scene_model *model, cg_scene_id scene_id)
{
	return (struct cg_scene_record *) cg_scene_model_find(model, scene_id);
}

enum cg_scene_result
cg_scene_model_create(struct cg_scene_model *model, cg_scene_id scene_id, cg_output_id output_id, uint32_t output_width,
		      uint32_t output_height)
{
	struct cg_scene_record *available = NULL;

	if (!model || scene_id == 0 || output_id == 0 || !output_size_valid(output_width, output_height)) {
		return CG_SCENE_INVALID;
	}
	for (size_t index = 0; index < CG_SCENE_CAPACITY; index++) {
		struct cg_scene_record *record = &model->scenes[index];
		if (!record->occupied) {
			if (!available) {
				available = record;
			}
			continue;
		}
		if (record->snapshot.scene_id == scene_id) {
			return record->snapshot.output_id == output_id && record->output_width == output_width &&
					       record->output_height == output_height
				       ? CG_SCENE_OK
				       : CG_SCENE_DUPLICATE_ID;
		}
		if (record->snapshot.output_id == output_id) {
			return CG_SCENE_DUPLICATE_ID;
		}
	}
	if (!available) {
		return CG_SCENE_CAPACITY_EXCEEDED;
	}
	memset(available, 0, sizeof(*available));
	available->occupied = true;
	available->output_width = output_width;
	available->output_height = output_height;
	available->snapshot_output_width = output_width;
	available->snapshot_output_height = output_height;
	available->snapshot.scene_id = scene_id;
	available->snapshot.output_id = output_id;
	model->scene_count++;
	return CG_SCENE_OK;
}

enum cg_scene_result
cg_scene_model_destroy(struct cg_scene_model *model, cg_scene_id scene_id)
{
	struct cg_scene_record *record = cg_scene_model_find_mutable(model, scene_id);

	if (!model || scene_id == 0) {
		return CG_SCENE_INVALID;
	}
	if (!record) {
		return CG_SCENE_NOT_FOUND;
	}
	memset(record, 0, sizeof(*record));
	model->scene_count--;
	return CG_SCENE_OK;
}

struct cg_scene_surface_state *
cg_scene_snapshot_find_surface_mutable(struct cg_scene_snapshot *snapshot, cg_surface_id surface_id)
{
	return (struct cg_scene_surface_state *) cg_scene_snapshot_find_surface(snapshot, surface_id);
}

const struct cg_scene_surface_state *
cg_scene_snapshot_find_surface(const struct cg_scene_snapshot *snapshot, cg_surface_id surface_id)
{
	if (!snapshot || surface_id == 0 || snapshot->surface_count > CG_SCENE_SURFACE_CAPACITY) {
		return NULL;
	}
	for (uint16_t index = 0; index < snapshot->surface_count; index++) {
		if (snapshot->surfaces[index].surface_id == surface_id) {
			return &snapshot->surfaces[index];
		}
	}
	return NULL;
}

static bool
edge_valid(enum cg_scene_resize_edge edge)
{
	return edge >= CG_SCENE_RESIZE_EDGE_LEFT && edge <= CG_SCENE_RESIZE_EDGE_BOTTOM;
}

static bool
cursor_valid(enum cg_scene_resize_cursor cursor)
{
	return cursor == CG_SCENE_RESIZE_CURSOR_COLUMN || cursor == CG_SCENE_RESIZE_CURSOR_ROW;
}

static enum cg_scene_result
validate_snapshot(const struct cg_scene_record *record, const struct cg_surface_registry *registry,
		  const struct cg_scene_snapshot *snapshot)
{
	if (!record || !registry || !snapshot || snapshot->scene_id == 0 || snapshot->output_id == 0 ||
	    snapshot->revision == 0 || !output_size_valid(snapshot->snapshot_output_width,
						    snapshot->snapshot_output_height) ||
	    snapshot->surface_count > CG_SCENE_SURFACE_CAPACITY ||
	    snapshot->resize_boundary_count > CG_SCENE_RESIZE_BOUNDARY_CAPACITY ||
	    snapshot->scene_id != record->snapshot.scene_id || snapshot->output_id != record->snapshot.output_id ||
	    (!snapshot->has_focused_surface && snapshot->focused_surface_id != 0)) {
		return CG_SCENE_INVALID;
	}
	for (uint16_t index = 0; index < snapshot->surface_count; index++) {
		const struct cg_scene_surface_state *state = &snapshot->surfaces[index];
		const struct cg_surface_registration *registration =
			cg_surface_registry_find(registry, snapshot->scene_id, state->surface_id);

		if (state->surface_id == 0 || !rect_valid(state->bounds) ||
		    (state->output_anchor_mask & ~CG_SCENE_OUTPUT_ANCHOR_MASK) != 0 ||
		    (state->has_clip && !rect_valid(state->clip)) ||
		    (!state->has_clip &&
		     (state->clip.x != 0 || state->clip.y != 0 || state->clip.width != 0 || state->clip.height != 0)) ||
		    (!state->has_parent && state->parent_surface_id != 0) ||
		    (state->accepts_input && !state->visible)) {
			return CG_SCENE_INVALID;
		}
		if (!registration || registration->state == CG_SURFACE_REGISTRATION_EXPIRED ||
		    registration->state == CG_SURFACE_REGISTRATION_RETIRED) {
			return CG_SCENE_SURFACE_NOT_REGISTERED;
		}
		if (registration->identity.has_parent != state->has_parent ||
		    registration->identity.parent_surface_id != state->parent_surface_id) {
			return CG_SCENE_SURFACE_PARENT_MISMATCH;
		}
		for (uint16_t other = 0; other < index; other++) {
			if (snapshot->surfaces[other].surface_id == state->surface_id) {
				return CG_SCENE_DUPLICATE_ID;
			}
		}
		if (state->has_parent && !cg_scene_snapshot_find_surface(snapshot, state->parent_surface_id)) {
			return CG_SCENE_SURFACE_PARENT_MISMATCH;
		}
	}
	if (snapshot->has_focused_surface) {
		const struct cg_scene_surface_state *focused =
			cg_scene_snapshot_find_surface(snapshot, snapshot->focused_surface_id);
		if (!focused || !focused->visible || !focused->accepts_input) {
			return CG_SCENE_FOCUS_INVALID;
		}
	}
	for (uint16_t index = 0; index < snapshot->resize_boundary_count; index++) {
		const struct cg_scene_resize_boundary *boundary = &snapshot->resize_boundaries[index];
		const struct cg_scene_surface_state *target =
			cg_scene_snapshot_find_surface(snapshot, boundary->target_surface_id);
		bool horizontal =
			boundary->edge == CG_SCENE_RESIZE_EDGE_LEFT || boundary->edge == CG_SCENE_RESIZE_EDGE_RIGHT;
		if (boundary->boundary_id == 0 || boundary->target_surface_id == 0 || !edge_valid(boundary->edge) ||
		    boundary->minimum_size == 0 || boundary->minimum_size > boundary->maximum_size ||
		    boundary->maximum_size > INT32_MAX || boundary->hit_slop > CG_SCENE_RESIZE_HIT_SLOP_MAX ||
		    !cursor_valid(boundary->cursor) || !target ||
		    (horizontal ? boundary->cursor != CG_SCENE_RESIZE_CURSOR_COLUMN
				: boundary->cursor != CG_SCENE_RESIZE_CURSOR_ROW) ||
		    (horizontal ? (uint32_t) target->bounds.width : (uint32_t) target->bounds.height) <
			    boundary->minimum_size ||
		    (horizontal ? (uint32_t) target->bounds.width : (uint32_t) target->bounds.height) >
			    boundary->maximum_size) {
			return CG_SCENE_BOUNDARY_INVALID;
		}
		for (uint16_t other = 0; other < index; other++) {
			if (snapshot->resize_boundaries[other].boundary_id == boundary->boundary_id) {
				return CG_SCENE_DUPLICATE_ID;
			}
		}
	}
	return CG_SCENE_OK;
}

enum cg_scene_result
cg_scene_model_apply(struct cg_scene_model *model, const struct cg_surface_registry *registry,
		     const struct cg_scene_snapshot *snapshot)
{
	struct cg_scene_record *record;
	enum cg_scene_result result;

	if (!model || !snapshot) {
		return CG_SCENE_INVALID;
	}
	record = cg_scene_model_find_mutable(model, snapshot->scene_id);
	if (!record) {
		model->rejected_snapshots++;
		return CG_SCENE_NOT_FOUND;
	}
	if (snapshot->revision < record->snapshot.revision) {
		model->rejected_snapshots++;
		return CG_SCENE_REVISION_STALE;
	}
	if (snapshot->revision == record->snapshot.revision && record->snapshot.revision != 0) {
		if (snapshot_equal(&record->snapshot, snapshot)) {
			return CG_SCENE_OK;
		}
		model->rejected_snapshots++;
		return CG_SCENE_REVISION_CONFLICT;
	}
	result = validate_snapshot(record, registry, snapshot);
	if (result != CG_SCENE_OK) {
		model->rejected_snapshots++;
		return result;
	}
	record->snapshot = *snapshot;
	record->snapshot_output_width = snapshot->snapshot_output_width;
	record->snapshot_output_height = snapshot->snapshot_output_height;
	model->applied_snapshots++;
	return CG_SCENE_OK;
}

enum cg_scene_result
cg_scene_model_resize_output(struct cg_scene_model *model, cg_scene_id scene_id, uint32_t output_width,
			     uint32_t output_height)
{
	struct cg_scene_record *record = cg_scene_model_find_mutable(model, scene_id);

	if (!model || scene_id == 0 || !output_size_valid(output_width, output_height)) {
		return CG_SCENE_INVALID;
	}
	if (!record) {
		return CG_SCENE_NOT_FOUND;
	}
	record->output_width = output_width;
	record->output_height = output_height;
	return CG_SCENE_OK;
}

static bool
intersect_rect(struct cg_scene_rect left, struct cg_scene_rect right, struct cg_scene_rect *result)
{
	int64_t x1 = left.x > right.x ? left.x : right.x;
	int64_t y1 = left.y > right.y ? left.y : right.y;
	int64_t left_x2 = (int64_t) left.x + left.width;
	int64_t right_x2 = (int64_t) right.x + right.width;
	int64_t left_y2 = (int64_t) left.y + left.height;
	int64_t right_y2 = (int64_t) right.y + right.height;
	int64_t x2 = left_x2 < right_x2 ? left_x2 : right_x2;
	int64_t y2 = left_y2 < right_y2 ? left_y2 : right_y2;

	if (!result || x2 <= x1 || y2 <= y1) {
		return false;
	}
	result->x = (int32_t) x1;
	result->y = (int32_t) y1;
	result->width = (int32_t) (x2 - x1);
	result->height = (int32_t) (y2 - y1);
	return true;
}

static bool
anchor_rect(struct cg_scene_rect source, uint8_t anchors, uint32_t old_width, uint32_t old_height, uint32_t new_width,
	    uint32_t new_height, struct cg_scene_rect *result)
{
	int64_t x = source.x;
	int64_t y = source.y;
	int64_t width = source.width;
	int64_t height = source.height;
	int64_t width_delta = (int64_t) new_width - old_width;
	int64_t height_delta = (int64_t) new_height - old_height;
	bool left = (anchors & CG_SCENE_OUTPUT_ANCHOR_LEFT) != 0;
	bool top = (anchors & CG_SCENE_OUTPUT_ANCHOR_TOP) != 0;
	bool right = (anchors & CG_SCENE_OUTPUT_ANCHOR_RIGHT) != 0;
	bool bottom = (anchors & CG_SCENE_OUTPUT_ANCHOR_BOTTOM) != 0;

	if (left && right) {
		width += width_delta;
	} else if (!left && right) {
		x += width_delta;
	}
	if (top && bottom) {
		height += height_delta;
	} else if (!top && bottom) {
		y += height_delta;
	}
	if (!result || x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX || width <= 0 ||
	    width > INT32_MAX || height <= 0 || height > INT32_MAX) {
		return false;
	}
	*result = (struct cg_scene_rect) {
		.x = (int32_t) x,
		.y = (int32_t) y,
		.width = (int32_t) width,
		.height = (int32_t) height,
	};
	return true;
}

bool
cg_scene_model_layout_surface(const struct cg_scene_model *model, cg_scene_id scene_id, cg_surface_id surface_id,
			      struct cg_scene_surface_state *state_out)
{
	const struct cg_scene_record *record = cg_scene_model_find(model, scene_id);
	const struct cg_scene_surface_state *state;

	if (!record || !state_out || record->snapshot_output_width == 0 || record->snapshot_output_height == 0) {
		return false;
	}
	state = cg_scene_snapshot_find_surface(&record->snapshot, surface_id);
	if (!state) {
		return false;
	}
	*state_out = *state;
	if (state->output_anchor_mask == 0 || (record->snapshot_output_width == record->output_width &&
					       record->snapshot_output_height == record->output_height)) {
		return true;
	}
	if (!anchor_rect(state->bounds, state->output_anchor_mask, record->snapshot_output_width,
			 record->snapshot_output_height, record->output_width, record->output_height,
			 &state_out->bounds)) {
		return false;
	}
	if (state->has_clip && !anchor_rect(state->clip, state->output_anchor_mask, record->snapshot_output_width,
					    record->snapshot_output_height, record->output_width, record->output_height,
					    &state_out->clip)) {
		return false;
	}
	return true;
}

bool
cg_scene_model_resolve_surface(const struct cg_scene_model *model, cg_scene_id scene_id, cg_surface_id surface_id,
			       struct cg_scene_rect *resolved_out)
{
	const struct cg_scene_record *record = cg_scene_model_find(model, scene_id);
	struct cg_scene_surface_state state;
	struct cg_scene_rect output;
	struct cg_scene_rect resolved;

	if (!record || !resolved_out) {
		return false;
	}
	if (!cg_scene_model_layout_surface(model, scene_id, surface_id, &state) || !state.visible) {
		return false;
	}
	output = (struct cg_scene_rect) {
		.width = (int32_t) record->output_width,
		.height = (int32_t) record->output_height,
	};
	if (!intersect_rect(state.bounds, output, &resolved)) {
		return false;
	}
	if (state.has_clip && !intersect_rect(resolved, state.clip, &resolved)) {
		return false;
	}
	*resolved_out = resolved;
	return true;
}

const struct cg_scene_surface_state *
cg_scene_model_hit_test(const struct cg_scene_model *model, const struct cg_surface_registry *registry,
			cg_scene_id scene_id, int32_t x, int32_t y)
{
	const struct cg_scene_record *record = cg_scene_model_find(model, scene_id);
	const struct cg_scene_surface_state *best = NULL;

	if (!record || !registry) {
		return NULL;
	}
	for (uint16_t index = 0; index < record->snapshot.surface_count; index++) {
		const struct cg_scene_surface_state *state = &record->snapshot.surfaces[index];
		const struct cg_surface_registration *registration =
			cg_surface_registry_find(registry, scene_id, state->surface_id);
		struct cg_scene_rect resolved;

		if (!state->visible || !state->accepts_input || !registration ||
		    registration->state != CG_SURFACE_REGISTRATION_ASSOCIATED ||
		    !cg_scene_model_resolve_surface(model, scene_id, state->surface_id, &resolved) || x < resolved.x ||
		    y < resolved.y || x >= (int64_t) resolved.x + resolved.width ||
		    y >= (int64_t) resolved.y + resolved.height) {
			continue;
		}
		if (!best || state->z_index > best->z_index ||
		    (state->z_index == best->z_index && state->surface_id > best->surface_id)) {
			best = state;
		}
	}
	return best;
}
