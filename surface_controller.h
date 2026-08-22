#ifndef CG_SURFACE_CONTROLLER_H
#define CG_SURFACE_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>

#include "surface_control_protocol.h"
#include "surface_registry.h"

typedef uint64_t (*cg_surface_controller_now_func)(void *data);

enum cg_surface_controller_event_type {
	CG_SURFACE_CONTROLLER_RESET,
	CG_SURFACE_CONTROLLER_RETIRED,
	CG_SURFACE_CONTROLLER_SCENE_APPLIED,
	CG_SURFACE_CONTROLLER_SCENE_DESTROYED,
	CG_SURFACE_CONTROLLER_OUTPUT_RESIZED,
	CG_SURFACE_CONTROLLER_BOUNDS_CHANGING,
	CG_SURFACE_CONTROLLER_BOUNDS_COMMITTED,
	CG_SURFACE_CONTROLLER_RESIZE_CANCELLED,
};

struct cg_surface_controller_event {
	enum cg_surface_controller_event_type type;
	cg_scene_id scene_id;
	cg_surface_id surface_id;
	cg_scene_revision revision;
	cg_resize_boundary_id boundary_id;
	struct cg_scene_rect bounds;
};

typedef void (*cg_surface_controller_event_func)(const struct cg_surface_controller_event *event, void *data);

struct cg_surface_controller {
	int listener_fd;
	char socket_path[108];
	struct wl_event_loop *event_loop;
	struct wl_event_source *listener_source;
	int client_fd;
	struct wl_event_source *client_source;
	struct cg_surface_registry *registry;
	struct cg_scene_model *scenes;
	cg_surface_controller_now_func now;
	void *now_data;
	cg_surface_controller_event_func event;
	void *event_data;
	bool accepting;
	enum cg_surface_control_parse_result last_parse_result;
	enum cg_surface_registry_result last_registry_result;
	enum cg_scene_result last_scene_result;
	uint64_t applied_messages;
	uint64_t rejected_messages;
	uint64_t receive_errors;
	uint64_t rejected_connections;
	uint64_t disconnects;
};

void cg_surface_controller_init(struct cg_surface_controller *controller);
bool cg_surface_controller_start(struct cg_surface_controller *controller, struct wl_event_loop *event_loop,
				 const char *path, const char *runtime_dir, struct cg_surface_registry *registry,
				 struct cg_scene_model *scenes, cg_surface_controller_now_func now, void *now_data,
				 cg_surface_controller_event_func event, void *event_data);
uint64_t cg_surface_controller_now(const struct cg_surface_controller *controller);
bool cg_surface_controller_notify_associated(struct cg_surface_controller *controller,
					     const struct cg_surface_identity *identity);
bool cg_surface_controller_notify_resize(struct cg_surface_controller *controller, const struct cg_resize_event *event);
void cg_surface_controller_stop(struct cg_surface_controller *controller);

#endif
