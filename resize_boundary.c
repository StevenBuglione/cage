/*
 * Cage: A Wayland kiosk.
 *
 * Generic compositor-owned interactive ResizeBoundary state machine.
 *
 * See the LICENSE file accompanying this file.
 */

#include <string.h>

#include "resize_boundary.h"

static const struct cg_scene_resize_boundary *
find_boundary(const struct cg_scene_snapshot *snapshot, cg_resize_boundary_id boundary_id)
{
	if (!snapshot || boundary_id == 0 || snapshot->resize_boundary_count > CG_SCENE_RESIZE_BOUNDARY_CAPACITY) {
		return NULL;
	}
	for (uint16_t index = 0; index < snapshot->resize_boundary_count; index++) {
		if (snapshot->resize_boundaries[index].boundary_id == boundary_id) {
			return &snapshot->resize_boundaries[index];
		}
	}
	return NULL;
}

static bool
surface_is_associated(const struct cg_surface_registry *registry, cg_scene_id scene_id, cg_surface_id surface_id)
{
	const struct cg_surface_registration *registration =
		cg_surface_registry_find(registry, scene_id, surface_id);
	return registration && registration->state == CG_SURFACE_REGISTRATION_ASSOCIATED;
}

static bool
point_hits_boundary(const struct cg_scene_resize_boundary *boundary, const struct cg_scene_surface_state *target,
		    struct cg_scene_rect resolved, double x, double y)
{
	double line;
	double distance;

	switch (boundary->edge) {
	case CG_SCENE_RESIZE_EDGE_LEFT:
		line = target->bounds.x;
		if (line < resolved.x || line > (int64_t) resolved.x + resolved.width) {
			return false;
		}
		distance = x - line;
		return y >= resolved.y && y < (int64_t) resolved.y + resolved.height &&
		       distance >= -(double) boundary->hit_slop && distance <= boundary->hit_slop;
	case CG_SCENE_RESIZE_EDGE_RIGHT:
		line = (int64_t) target->bounds.x + target->bounds.width;
		if (line < resolved.x || line > (int64_t) resolved.x + resolved.width) {
			return false;
		}
		distance = x - line;
		return y >= resolved.y && y < (int64_t) resolved.y + resolved.height &&
		       distance >= -(double) boundary->hit_slop && distance <= boundary->hit_slop;
	case CG_SCENE_RESIZE_EDGE_TOP:
		line = target->bounds.y;
		if (line < resolved.y || line > (int64_t) resolved.y + resolved.height) {
			return false;
		}
		distance = y - line;
		return x >= resolved.x && x < (int64_t) resolved.x + resolved.width &&
		       distance >= -(double) boundary->hit_slop && distance <= boundary->hit_slop;
	case CG_SCENE_RESIZE_EDGE_BOTTOM:
		line = (int64_t) target->bounds.y + target->bounds.height;
		if (line < resolved.y || line > (int64_t) resolved.y + resolved.height) {
			return false;
		}
		distance = y - line;
		return x >= resolved.x && x < (int64_t) resolved.x + resolved.width &&
		       distance >= -(double) boundary->hit_slop && distance <= boundary->hit_slop;
	}
	return false;
}

void
cg_resize_session_init(struct cg_resize_session *session)
{
	if (session) {
		memset(session, 0, sizeof(*session));
	}
}

bool
cg_resize_boundary_hit_test(const struct cg_scene_model *model, const struct cg_surface_registry *registry, double x,
			    double y, struct cg_resize_hit *hit_out)
{
	const struct cg_scene_surface_state *best_target = NULL;
	const struct cg_scene_resize_boundary *best_boundary = NULL;
	cg_scene_id best_scene_id = 0;

	if (!model || !registry || !hit_out) {
		return false;
	}
	for (size_t scene_index = 0; scene_index < CG_SCENE_CAPACITY; scene_index++) {
		const struct cg_scene_record *record = &model->scenes[scene_index];
		if (!record->occupied) {
			continue;
		}
		for (uint16_t index = 0; index < record->snapshot.resize_boundary_count; index++) {
			const struct cg_scene_resize_boundary *boundary = &record->snapshot.resize_boundaries[index];
			const struct cg_scene_surface_state *target =
				cg_scene_snapshot_find_surface(&record->snapshot, boundary->target_surface_id);
			struct cg_scene_rect resolved;
			if (!boundary->enabled || !boundary->visible || !target || !target->visible ||
			    !surface_is_associated(registry, record->snapshot.scene_id, target->surface_id) ||
			    !cg_scene_model_resolve_surface(model, record->snapshot.scene_id, target->surface_id, &resolved) ||
			    !point_hits_boundary(boundary, target, resolved, x, y)) {
				continue;
			}
			if (!best_target || target->z_index > best_target->z_index ||
			    (target->z_index == best_target->z_index && boundary->boundary_id > best_boundary->boundary_id)) {
				best_target = target;
				best_boundary = boundary;
				best_scene_id = record->snapshot.scene_id;
			}
		}
	}
	if (!best_boundary) {
		return false;
	}
	*hit_out = (struct cg_resize_hit) {
		.scene_id = best_scene_id,
		.boundary_id = best_boundary->boundary_id,
		.surface_id = best_boundary->target_surface_id,
		.edge = best_boundary->edge,
		.cursor = best_boundary->cursor,
	};
	return true;
}

bool
cg_resize_session_begin(struct cg_resize_session *session, const struct cg_scene_model *model,
			const struct cg_surface_registry *registry, double x, double y, uint64_t now_ms)
{
	struct cg_resize_hit hit;
	const struct cg_scene_record *record;
	const struct cg_scene_resize_boundary *boundary;
	const struct cg_scene_surface_state *target;

	if (!session || session->active || !cg_resize_boundary_hit_test(model, registry, x, y, &hit)) {
		return false;
	}
	record = cg_scene_model_find(model, hit.scene_id);
	boundary = find_boundary(&record->snapshot, hit.boundary_id);
	target = cg_scene_snapshot_find_surface(&record->snapshot, hit.surface_id);
	session->active = true;
	session->hit = hit;
	session->revision = record->snapshot.revision;
	session->start_x = x;
	session->start_y = y;
	session->committed_bounds = target->bounds;
	session->minimum_size = boundary->minimum_size;
	session->maximum_size = boundary->maximum_size;
	session->output_width = record->output_width;
	session->output_height = record->output_height;
	session->emitted_change = false;
	session->last_event_ms = now_ms;
	return true;
}

static void
set_event(const struct cg_resize_session *session, enum cg_resize_event_type type, struct cg_scene_rect bounds,
	  struct cg_resize_event *event_out)
{
	if (!event_out) {
		return;
	}
	*event_out = (struct cg_resize_event) {
		.type = type,
		.scene_id = session->hit.scene_id,
		.revision = session->revision,
		.boundary_id = session->hit.boundary_id,
		.surface_id = session->hit.surface_id,
		.bounds = bounds,
	};
}

static struct cg_scene_surface_state *
active_target(struct cg_resize_session *session, struct cg_scene_model *model,
	      const struct cg_surface_registry *registry)
{
	struct cg_scene_record *record = cg_scene_model_find_mutable(model, session->hit.scene_id);
	const struct cg_scene_resize_boundary *boundary;
	struct cg_scene_surface_state *target;

	if (!record || record->snapshot.revision != session->revision || record->output_width != session->output_width ||
	    record->output_height != session->output_height) {
		return NULL;
	}
	boundary = find_boundary(&record->snapshot, session->hit.boundary_id);
	target = cg_scene_snapshot_find_surface_mutable(&record->snapshot, session->hit.surface_id);
	if (!boundary || !boundary->enabled || !boundary->visible ||
	    boundary->target_surface_id != session->hit.surface_id || !target || !target->visible ||
	    !surface_is_associated(registry, session->hit.scene_id, session->hit.surface_id)) {
		return NULL;
	}
	return target;
}

static int32_t
clamp_size(int64_t size, uint32_t minimum, uint32_t maximum)
{
	if (size < minimum) {
		return (int32_t) minimum;
	}
	if (size > maximum) {
		return (int32_t) maximum;
	}
	return (int32_t) size;
}

bool
cg_resize_session_update(struct cg_resize_session *session, struct cg_scene_model *model,
			 const struct cg_surface_registry *registry, double x, double y, uint64_t now_ms,
			 struct cg_resize_event *event_out)
{
	struct cg_scene_surface_state *target;
	struct cg_scene_rect next;
	int64_t delta;
	int32_t size;

	if (event_out) {
		memset(event_out, 0, sizeof(*event_out));
	}
	if (!session || !session->active || !model || !registry) {
		return false;
	}
	target = active_target(session, model, registry);
	if (!target) {
		return cg_resize_session_cancel(session, model, event_out);
	}
	next = session->committed_bounds;
	switch (session->hit.edge) {
	case CG_SCENE_RESIZE_EDGE_LEFT:
		delta = (int64_t) (x - session->start_x);
		size = clamp_size((int64_t) session->committed_bounds.width - delta, session->minimum_size,
				  session->maximum_size);
		next.x = session->committed_bounds.x + session->committed_bounds.width - size;
		next.width = size;
		break;
	case CG_SCENE_RESIZE_EDGE_RIGHT:
		delta = (int64_t) (x - session->start_x);
		next.width = clamp_size((int64_t) session->committed_bounds.width + delta, session->minimum_size,
					session->maximum_size);
		break;
	case CG_SCENE_RESIZE_EDGE_TOP:
		delta = (int64_t) (y - session->start_y);
		size = clamp_size((int64_t) session->committed_bounds.height - delta, session->minimum_size,
				  session->maximum_size);
		next.y = session->committed_bounds.y + session->committed_bounds.height - size;
		next.height = size;
		break;
	case CG_SCENE_RESIZE_EDGE_BOTTOM:
		delta = (int64_t) (y - session->start_y);
		next.height = clamp_size((int64_t) session->committed_bounds.height + delta, session->minimum_size,
					 session->maximum_size);
		break;
	}
	set_event(session, CG_RESIZE_EVENT_NONE, next, event_out);
	if (target->bounds.x == next.x && target->bounds.y == next.y && target->bounds.width == next.width &&
	    target->bounds.height == next.height) {
		return true;
	}
	target->bounds = next;
	if (!session->emitted_change || now_ms < session->last_event_ms ||
	    now_ms - session->last_event_ms >= CG_RESIZE_EVENT_INTERVAL_MS) {
		session->emitted_change = true;
		session->last_event_ms = now_ms;
		set_event(session, CG_RESIZE_EVENT_BOUNDS_CHANGING, next, event_out);
	}
	return true;
}

bool
cg_resize_session_commit(struct cg_resize_session *session, struct cg_scene_model *model,
			 const struct cg_surface_registry *registry, struct cg_resize_event *event_out)
{
	struct cg_scene_surface_state *target;
	struct cg_scene_rect bounds;

	if (event_out) {
		memset(event_out, 0, sizeof(*event_out));
	}
	if (!session || !session->active || !model || !registry) {
		return false;
	}
	target = active_target(session, model, registry);
	bounds = target ? target->bounds : session->committed_bounds;
	set_event(session, target ? CG_RESIZE_EVENT_BOUNDS_COMMITTED : CG_RESIZE_EVENT_CANCELLED, bounds, event_out);
	cg_resize_session_init(session);
	return true;
}

bool
cg_resize_session_cancel(struct cg_resize_session *session, struct cg_scene_model *model,
			 struct cg_resize_event *event_out)
{
	struct cg_scene_record *record;
	struct cg_scene_surface_state *target;

	if (event_out) {
		memset(event_out, 0, sizeof(*event_out));
	}
	if (!session || !session->active) {
		return false;
	}
	record = model ? cg_scene_model_find_mutable(model, session->hit.scene_id) : NULL;
	target = record && record->snapshot.revision == session->revision
			 ? cg_scene_snapshot_find_surface_mutable(&record->snapshot, session->hit.surface_id)
			 : NULL;
	if (target) {
		target->bounds = session->committed_bounds;
	}
	set_event(session, CG_RESIZE_EVENT_CANCELLED, session->committed_bounds, event_out);
	cg_resize_session_init(session);
	return true;
}
