#ifndef CG_SURFACE_CONTROL_PROTOCOL_H
#define CG_SURFACE_CONTROL_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "surface_registry.h"

#define CG_SURFACE_CONTROL_VERSION 1
#define CG_SURFACE_CONTROL_HEADER_SIZE 8
#define CG_SURFACE_CONTROL_REGISTER_SIZE 72
#define CG_SURFACE_CONTROL_UNREGISTER_SIZE 24
#define CG_SURFACE_CONTROL_RESET_SIZE 8
#define CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE CG_SURFACE_CONTROL_REGISTER_SIZE

enum cg_surface_control_message_type {
	CG_SURFACE_CONTROL_REGISTER = 1,
	CG_SURFACE_CONTROL_UNREGISTER = 2,
	CG_SURFACE_CONTROL_RESET = 3,
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

struct cg_surface_control_message {
	enum cg_surface_control_message_type type;
	union {
		struct cg_surface_registration_request registration;
		struct cg_surface_control_unregister unregistration;
	};
};

enum cg_surface_control_parse_result cg_surface_control_parse(const uint8_t *bytes, size_t size,
							      struct cg_surface_control_message *message_out);
bool cg_surface_control_encode_register(const struct cg_surface_registration_request *request, uint8_t *bytes_out,
					size_t capacity, size_t *size_out);
bool cg_surface_control_encode_unregister(const struct cg_surface_control_unregister *request, uint8_t *bytes_out,
					  size_t capacity, size_t *size_out);
bool cg_surface_control_encode_reset(uint8_t *bytes_out, size_t capacity, size_t *size_out);

#endif
