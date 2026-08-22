#ifndef CG_RESIZE_BOUNDARY_H
#define CG_RESIZE_BOUNDARY_H

#include <stdbool.h>
#include <stdint.h>

#include "scene_model.h"
#include "surface_registry.h"

#define CG_RESIZE_EVENT_INTERVAL_MS 16

enum cg_resize_event_type {
	CG_RESIZE_EVENT_NONE,
	CG_RESIZE_EVENT_BOUNDS_CHANGING,
	CG_RESIZE_EVENT_BOUNDS_COMMITTED,
	CG_RESIZE_EVENT_CANCELLED,
};

struct cg_resize_event {
	enum cg_resize_event_type type;
	cg_scene_id scene_id;
	cg_scene_revision revision;
	cg_resize_boundary_id boundary_id;
	cg_surface_id surface_id;
	struct cg_scene_rect bounds;
};

struct cg_resize_hit {
	cg_scene_id scene_id;
	cg_resize_boundary_id boundary_id;
	cg_surface_id surface_id;
	enum cg_scene_resize_edge edge;
	enum cg_scene_resize_cursor cursor;
};

struct cg_resize_session {
	bool active;
	struct cg_resize_hit hit;
	cg_scene_revision revision;
	double start_x;
	double start_y;
	struct cg_scene_rect committed_bounds;
	uint32_t minimum_size;
	uint32_t maximum_size;
	uint32_t output_width;
	uint32_t output_height;
	bool emitted_change;
	uint64_t last_event_ms;
};

void cg_resize_session_init(struct cg_resize_session *session);
bool cg_resize_boundary_hit_test(const struct cg_scene_model *model, const struct cg_surface_registry *registry,
				 double x, double y, struct cg_resize_hit *hit_out);
bool cg_resize_session_begin(struct cg_resize_session *session, const struct cg_scene_model *model,
			     const struct cg_surface_registry *registry, double x, double y, uint64_t now_ms);
bool cg_resize_session_update(struct cg_resize_session *session, struct cg_scene_model *model,
			      const struct cg_surface_registry *registry, double x, double y, uint64_t now_ms,
			      struct cg_resize_event *event_out);
bool cg_resize_session_commit(struct cg_resize_session *session, struct cg_scene_model *model,
			      struct cg_resize_event *event_out);
bool cg_resize_session_cancel(struct cg_resize_session *session, struct cg_scene_model *model,
			      struct cg_resize_event *event_out);

#endif
