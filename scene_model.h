#ifndef CG_SCENE_MODEL_H
#define CG_SCENE_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "surface_registry.h"

#define CG_SCENE_CAPACITY 8
#define CG_SCENE_SURFACE_CAPACITY CG_SURFACE_REGISTRY_CAPACITY
#define CG_SCENE_RESIZE_BOUNDARY_CAPACITY 128
#define CG_SCENE_RESIZE_HIT_SLOP_MAX 128
#define CG_SCENE_OUTPUT_ANCHOR_LEFT (1u << 0)
#define CG_SCENE_OUTPUT_ANCHOR_TOP (1u << 1)
#define CG_SCENE_OUTPUT_ANCHOR_RIGHT (1u << 2)
#define CG_SCENE_OUTPUT_ANCHOR_BOTTOM (1u << 3)
#define CG_SCENE_OUTPUT_ANCHOR_MASK                                                                                    \
	(CG_SCENE_OUTPUT_ANCHOR_LEFT | CG_SCENE_OUTPUT_ANCHOR_TOP | CG_SCENE_OUTPUT_ANCHOR_RIGHT |                     \
	 CG_SCENE_OUTPUT_ANCHOR_BOTTOM)

typedef uint64_t cg_output_id;
typedef uint64_t cg_scene_revision;
typedef uint64_t cg_resize_boundary_id;

struct cg_scene_rect {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};

struct cg_scene_surface_state {
	cg_surface_id surface_id;
	struct cg_scene_rect bounds;
	bool has_clip;
	struct cg_scene_rect clip;
	int32_t z_index;
	bool visible;
	bool accepts_input;
	bool has_parent;
	cg_surface_id parent_surface_id;
	bool modal;
	uint8_t output_anchor_mask;
};

enum cg_scene_resize_edge {
	CG_SCENE_RESIZE_EDGE_LEFT = 1,
	CG_SCENE_RESIZE_EDGE_RIGHT = 2,
	CG_SCENE_RESIZE_EDGE_TOP = 3,
	CG_SCENE_RESIZE_EDGE_BOTTOM = 4,
};

enum cg_scene_resize_cursor {
	CG_SCENE_RESIZE_CURSOR_COLUMN = 1,
	CG_SCENE_RESIZE_CURSOR_ROW = 2,
};

struct cg_scene_resize_boundary {
	cg_resize_boundary_id boundary_id;
	cg_surface_id target_surface_id;
	enum cg_scene_resize_edge edge;
	uint32_t minimum_size;
	uint32_t maximum_size;
	uint32_t hit_slop;
	enum cg_scene_resize_cursor cursor;
	bool enabled;
	bool visible;
};

struct cg_scene_snapshot {
	cg_scene_id scene_id;
	cg_output_id output_id;
	cg_scene_revision revision;
	uint32_t snapshot_output_width;
	uint32_t snapshot_output_height;
	uint16_t surface_count;
	uint16_t resize_boundary_count;
	bool has_focused_surface;
	cg_surface_id focused_surface_id;
	struct cg_scene_surface_state surfaces[CG_SCENE_SURFACE_CAPACITY];
	struct cg_scene_resize_boundary resize_boundaries[CG_SCENE_RESIZE_BOUNDARY_CAPACITY];
};

struct cg_scene_record {
	bool occupied;
	uint32_t output_width;
	uint32_t output_height;
	uint32_t snapshot_output_width;
	uint32_t snapshot_output_height;
	struct cg_scene_snapshot snapshot;
};

struct cg_scene_model {
	struct cg_scene_record scenes[CG_SCENE_CAPACITY];
	size_t scene_count;
	uint64_t applied_snapshots;
	uint64_t rejected_snapshots;
};

enum cg_scene_result {
	CG_SCENE_OK,
	CG_SCENE_INVALID,
	CG_SCENE_CAPACITY_EXCEEDED,
	CG_SCENE_NOT_FOUND,
	CG_SCENE_DUPLICATE_ID,
	CG_SCENE_REVISION_STALE,
	CG_SCENE_REVISION_CONFLICT,
	CG_SCENE_SURFACE_NOT_REGISTERED,
	CG_SCENE_SURFACE_PARENT_MISMATCH,
	CG_SCENE_FOCUS_INVALID,
	CG_SCENE_BOUNDARY_INVALID,
};

void cg_scene_model_init(struct cg_scene_model *model);
void cg_scene_model_reset(struct cg_scene_model *model);
enum cg_scene_result cg_scene_model_create(struct cg_scene_model *model, cg_scene_id scene_id, cg_output_id output_id,
					   uint32_t output_width, uint32_t output_height);
enum cg_scene_result cg_scene_model_destroy(struct cg_scene_model *model, cg_scene_id scene_id);
enum cg_scene_result cg_scene_model_apply(struct cg_scene_model *model, const struct cg_surface_registry *registry,
					  const struct cg_scene_snapshot *snapshot);
enum cg_scene_result cg_scene_model_resize_output(struct cg_scene_model *model, cg_scene_id scene_id,
						  uint32_t output_width, uint32_t output_height);
const struct cg_scene_record *cg_scene_model_find(const struct cg_scene_model *model, cg_scene_id scene_id);
struct cg_scene_record *cg_scene_model_find_mutable(struct cg_scene_model *model, cg_scene_id scene_id);
const struct cg_scene_surface_state *cg_scene_snapshot_find_surface(const struct cg_scene_snapshot *snapshot,
								    cg_surface_id surface_id);
struct cg_scene_surface_state *cg_scene_snapshot_find_surface_mutable(struct cg_scene_snapshot *snapshot,
								      cg_surface_id surface_id);
bool cg_scene_model_resolve_surface(const struct cg_scene_model *model, cg_scene_id scene_id, cg_surface_id surface_id,
				    struct cg_scene_rect *resolved_out);
bool cg_scene_model_layout_surface(const struct cg_scene_model *model, cg_scene_id scene_id, cg_surface_id surface_id,
				   struct cg_scene_surface_state *state_out);
const struct cg_scene_surface_state *cg_scene_model_hit_test(const struct cg_scene_model *model,
							     const struct cg_surface_registry *registry,
							     cg_scene_id scene_id, int32_t x, int32_t y);

#endif
