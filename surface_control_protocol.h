#ifndef CG_SURFACE_CONTROL_PROTOCOL_H
#define CG_SURFACE_CONTROL_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "resize_boundary.h"
#include "scene_model.h"
#include "surface_registry.h"

#define CG_SURFACE_CONTROL_VERSION 1
#define CG_SURFACE_CONTROL_HEADER_SIZE 8
#define CG_SURFACE_CONTROL_REGISTER_SIZE 72
#define CG_SURFACE_CONTROL_UNREGISTER_SIZE 24
#define CG_SURFACE_CONTROL_RESET_SIZE 8
#define CG_SURFACE_CONTROL_REGISTERED_SIZE 40
#define CG_SURFACE_CONTROL_ASSOCIATED_SIZE 40
#define CG_SURFACE_CONTROL_CREATE_SCENE_SIZE 32
#define CG_SURFACE_CONTROL_DESTROY_SCENE_SIZE 16
#define CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE 32
#define CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE 56
#define CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE 48
#define CG_SURFACE_CONTROL_SURFACE_STATE_SIZE 64
#define CG_SURFACE_CONTROL_RESIZE_BOUNDARY_SIZE 40
#define CG_SURFACE_CONTROL_APPLY_SCENE_MAX_SIZE                                                                        \
	(CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE +                                                                  \
	 CG_SCENE_SURFACE_CAPACITY * CG_SURFACE_CONTROL_SURFACE_STATE_SIZE +                                           \
	 CG_SCENE_RESIZE_BOUNDARY_CAPACITY * CG_SURFACE_CONTROL_RESIZE_BOUNDARY_SIZE)
#define CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE CG_SURFACE_CONTROL_APPLY_SCENE_MAX_SIZE

enum cg_surface_control_message_type {
	CG_SURFACE_CONTROL_REGISTER = 1,
	CG_SURFACE_CONTROL_UNREGISTER = 2,
	CG_SURFACE_CONTROL_RESET = 3,
	CG_SURFACE_CONTROL_CREATE_SCENE = 4,
	CG_SURFACE_CONTROL_DESTROY_SCENE = 5,
	CG_SURFACE_CONTROL_APPLY_SCENE = 6,
	CG_SURFACE_CONTROL_RESIZE_OUTPUT = 7,
	CG_SURFACE_CONTROL_REGISTERED = 0x80,
	CG_SURFACE_CONTROL_ASSOCIATED = 0x81,
	CG_SURFACE_CONTROL_BOUNDS_CHANGING = 0x82,
	CG_SURFACE_CONTROL_BOUNDS_COMMITTED = 0x83,
	CG_SURFACE_CONTROL_RESIZE_CANCELLED = 0x84,
	CG_SURFACE_CONTROL_OUTPUT_CHANGED = 0x85,
};

enum cg_surface_control_parse_result {
	CG_SURFACE_CONTROL_PARSE_OK,
	CG_SURFACE_CONTROL_PARSE_INVALID_SIZE,
	CG_SURFACE_CONTROL_PARSE_INVALID_MAGIC,
	CG_SURFACE_CONTROL_PARSE_UNSUPPORTED_VERSION,
	CG_SURFACE_CONTROL_PARSE_UNKNOWN_TYPE,
	CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED,
	CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS,
};

struct cg_surface_control_unregister {
	cg_scene_id scene_id;
	cg_surface_id surface_id;
};

struct cg_surface_control_create_scene {
	cg_scene_id scene_id;
	cg_output_id output_id;
	uint32_t output_width;
	uint32_t output_height;
};

struct cg_surface_control_destroy_scene {
	cg_scene_id scene_id;
};

struct cg_surface_control_resize_output {
	cg_scene_id scene_id;
	cg_output_id output_id;
	uint32_t output_width;
	uint32_t output_height;
};

struct cg_surface_control_message {
	enum cg_surface_control_message_type type;
	union {
		struct cg_surface_registration_request registration;
		struct cg_surface_control_unregister unregistration;
		struct cg_surface_control_create_scene create_scene;
		struct cg_surface_control_destroy_scene destroy_scene;
		struct cg_scene_snapshot scene_snapshot;
		struct cg_surface_control_resize_output resize_output;
	};
};

enum cg_surface_control_parse_result cg_surface_control_parse(const uint8_t *bytes, size_t size,
							      struct cg_surface_control_message *message_out);
bool cg_surface_control_encode_register(const struct cg_surface_registration_request *request, uint8_t *bytes_out,
					size_t capacity, size_t *size_out);
bool cg_surface_control_encode_unregister(const struct cg_surface_control_unregister *request, uint8_t *bytes_out,
					  size_t capacity, size_t *size_out);
bool cg_surface_control_encode_reset(uint8_t *bytes_out, size_t capacity, size_t *size_out);
bool cg_surface_control_encode_registered(const struct cg_surface_registration_request *request, uint8_t *bytes_out,
					  size_t capacity, size_t *size_out);
bool cg_surface_control_encode_associated(const struct cg_surface_identity *identity, uint8_t *bytes_out,
					  size_t capacity, size_t *size_out);
bool cg_surface_control_encode_create_scene(const struct cg_surface_control_create_scene *request, uint8_t *bytes_out,
					    size_t capacity, size_t *size_out);
bool cg_surface_control_encode_destroy_scene(const struct cg_surface_control_destroy_scene *request, uint8_t *bytes_out,
					     size_t capacity, size_t *size_out);
bool cg_surface_control_encode_apply_scene(const struct cg_scene_snapshot *snapshot, uint8_t *bytes_out,
					   size_t capacity, size_t *size_out);
bool cg_surface_control_encode_resize_output(const struct cg_surface_control_resize_output *request, uint8_t *bytes_out,
					     size_t capacity, size_t *size_out);
bool cg_surface_control_encode_output_changed(const struct cg_surface_control_resize_output *event, uint8_t *bytes_out,
					      size_t capacity, size_t *size_out);
bool cg_surface_control_encode_resize_event(const struct cg_resize_event *event, uint8_t *bytes_out, size_t capacity,
					    size_t *size_out);

#endif
