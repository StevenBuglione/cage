/*
 * Cage: A Wayland kiosk.
 *
 * Bounded temporary binary protocol for M1 surface registration.
 * M2 replaces this transport with the versioned scene protobuf service.
 *
 * See the LICENSE file accompanying this file.
 */

#include <string.h>

#include "surface_control_protocol.h"

static const uint8_t protocol_magic[4] = {'L', 'S', 'C', '1'};

static uint16_t
read_u16(const uint8_t *bytes)
{
	return (uint16_t) ((uint16_t) bytes[0] << 8 | bytes[1]);
}

static uint32_t
read_u32(const uint8_t *bytes)
{
	return (uint32_t) bytes[0] << 24 | (uint32_t) bytes[1] << 16 | (uint32_t) bytes[2] << 8 | bytes[3];
}

static int32_t
read_i32(const uint8_t *bytes)
{
	return (int32_t) read_u32(bytes);
}

static uint64_t
read_u64(const uint8_t *bytes)
{
	uint64_t value = 0;

	for (size_t index = 0; index < 8; index++) {
		value = value << 8 | bytes[index];
	}
	return value;
}

static void
write_u16(uint8_t *bytes, uint16_t value)
{
	bytes[0] = (uint8_t) (value >> 8);
	bytes[1] = (uint8_t) value;
}

static void
write_u32(uint8_t *bytes, uint32_t value)
{
	bytes[0] = (uint8_t) (value >> 24);
	bytes[1] = (uint8_t) (value >> 16);
	bytes[2] = (uint8_t) (value >> 8);
	bytes[3] = (uint8_t) value;
}

static void
write_i32(uint8_t *bytes, int32_t value)
{
	write_u32(bytes, (uint32_t) value);
}

static void
write_u64(uint8_t *bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; index++) {
		bytes[7 - index] = (uint8_t) (value >> (index * 8));
	}
}

static void
write_header(uint8_t *bytes, enum cg_surface_control_message_type type, uint16_t size)
{
	memcpy(bytes, protocol_magic, sizeof(protocol_magic));
	bytes[4] = CG_SURFACE_CONTROL_VERSION;
	bytes[5] = (uint8_t) type;
	write_u16(bytes + 6, size);
}

static bool
registration_fields_valid(const struct cg_surface_registration_request *request)
{
	return request && request->scene_id != 0 && request->surface_id != 0 &&
	       cg_surface_token_is_valid(&request->token) && cg_surface_kind_is_valid(request->kind) &&
	       request->association_timeout_ms > 0 &&
	       request->association_timeout_ms <= CG_SURFACE_ASSOCIATION_TIMEOUT_MAX_MS &&
	       ((request->has_parent && request->parent_surface_id != 0 &&
		 request->parent_surface_id != request->surface_id) ||
		(!request->has_parent && request->parent_surface_id == 0)) &&
	       (request->kind != CG_SURFACE_KIND_POPUP || request->has_parent);
}

static bool
bytes_are_zero(const uint8_t *bytes, size_t size)
{
	for (size_t index = 0; index < size; index++) {
		if (bytes[index] != 0) {
			return false;
		}
	}
	return true;
}

static bool
scene_header_fields_valid(const struct cg_scene_snapshot *snapshot)
{
	return snapshot && snapshot->scene_id != 0 && snapshot->output_id != 0 && snapshot->revision != 0 &&
	       snapshot->surface_count <= CG_SCENE_SURFACE_CAPACITY &&
	       snapshot->resize_boundary_count <= CG_SCENE_RESIZE_BOUNDARY_CAPACITY &&
	       (snapshot->has_focused_surface ? snapshot->focused_surface_id != 0 : snapshot->focused_surface_id == 0);
}

static size_t
scene_message_size(const struct cg_scene_snapshot *snapshot)
{
	return CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE +
	       (size_t) snapshot->surface_count * CG_SURFACE_CONTROL_SURFACE_STATE_SIZE +
	       (size_t) snapshot->resize_boundary_count * CG_SURFACE_CONTROL_RESIZE_BOUNDARY_SIZE;
}

enum cg_surface_control_parse_result
cg_surface_control_parse(const uint8_t *bytes, size_t size, struct cg_surface_control_message *message_out)
{
	uint16_t declared_size;

	if (!bytes || !message_out || size < CG_SURFACE_CONTROL_HEADER_SIZE ||
	    size > CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE) {
		return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
	}
	if (memcmp(bytes, protocol_magic, sizeof(protocol_magic)) != 0) {
		return CG_SURFACE_CONTROL_PARSE_INVALID_MAGIC;
	}
	if (bytes[4] != CG_SURFACE_CONTROL_VERSION) {
		return CG_SURFACE_CONTROL_PARSE_UNSUPPORTED_VERSION;
	}
	declared_size = read_u16(bytes + 6);
	if (declared_size != size) {
		return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
	}

	memset(message_out, 0, sizeof(*message_out));
	switch (bytes[5]) {
	case CG_SURFACE_CONTROL_REGISTER: {
		struct cg_surface_registration_request *request = &message_out->registration;
		if (size != CG_SURFACE_CONTROL_REGISTER_SIZE) {
			return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
		}
		if ((bytes[25] & ~1u) != 0 || bytes[26] != 0 || bytes[27] != 0) {
			return CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED;
		}
		message_out->type = CG_SURFACE_CONTROL_REGISTER;
		request->scene_id = read_u64(bytes + 8);
		request->surface_id = read_u64(bytes + 16);
		request->kind = (enum cg_surface_kind) bytes[24];
		request->has_parent = (bytes[25] & 1u) != 0;
		request->parent_surface_id = read_u64(bytes + 28);
		request->association_timeout_ms = read_u32(bytes + 36);
		memcpy(request->token.bytes, bytes + 40, CG_SURFACE_TOKEN_SIZE);
		if (!registration_fields_valid(request)) {
			memset(message_out, 0, sizeof(*message_out));
			return CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS;
		}
		return CG_SURFACE_CONTROL_PARSE_OK;
	}
	case CG_SURFACE_CONTROL_UNREGISTER:
		if (size != CG_SURFACE_CONTROL_UNREGISTER_SIZE) {
			return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
		}
		message_out->type = CG_SURFACE_CONTROL_UNREGISTER;
		message_out->unregistration.scene_id = read_u64(bytes + 8);
		message_out->unregistration.surface_id = read_u64(bytes + 16);
		if (message_out->unregistration.scene_id == 0 || message_out->unregistration.surface_id == 0) {
			memset(message_out, 0, sizeof(*message_out));
			return CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS;
		}
		return CG_SURFACE_CONTROL_PARSE_OK;
	case CG_SURFACE_CONTROL_RESET:
		if (size != CG_SURFACE_CONTROL_RESET_SIZE) {
			return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
		}
		message_out->type = CG_SURFACE_CONTROL_RESET;
		return CG_SURFACE_CONTROL_PARSE_OK;
	case CG_SURFACE_CONTROL_CREATE_SCENE:
		if (size != CG_SURFACE_CONTROL_CREATE_SCENE_SIZE) {
			return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
		}
		message_out->type = CG_SURFACE_CONTROL_CREATE_SCENE;
		message_out->create_scene.scene_id = read_u64(bytes + 8);
		message_out->create_scene.output_id = read_u64(bytes + 16);
		message_out->create_scene.output_width = read_u32(bytes + 24);
		message_out->create_scene.output_height = read_u32(bytes + 28);
		if (message_out->create_scene.scene_id == 0 || message_out->create_scene.output_id == 0 ||
		    message_out->create_scene.output_width == 0 || message_out->create_scene.output_height == 0) {
			memset(message_out, 0, sizeof(*message_out));
			return CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS;
		}
		return CG_SURFACE_CONTROL_PARSE_OK;
	case CG_SURFACE_CONTROL_DESTROY_SCENE:
		if (size != CG_SURFACE_CONTROL_DESTROY_SCENE_SIZE) {
			return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
		}
		message_out->type = CG_SURFACE_CONTROL_DESTROY_SCENE;
		message_out->destroy_scene.scene_id = read_u64(bytes + 8);
		if (message_out->destroy_scene.scene_id == 0) {
			memset(message_out, 0, sizeof(*message_out));
			return CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS;
		}
		return CG_SURFACE_CONTROL_PARSE_OK;
	case CG_SURFACE_CONTROL_APPLY_SCENE: {
		struct cg_scene_snapshot *snapshot = &message_out->scene_snapshot;
		size_t offset = CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE;
		if (size < CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE || (bytes[36] & ~1u) != 0 ||
		    !bytes_are_zero(bytes + 37, 3)) {
			return size < CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE
				       ? CG_SURFACE_CONTROL_PARSE_INVALID_SIZE
				       : CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED;
		}
		message_out->type = CG_SURFACE_CONTROL_APPLY_SCENE;
		snapshot->scene_id = read_u64(bytes + 8);
		snapshot->output_id = read_u64(bytes + 16);
		snapshot->revision = read_u64(bytes + 24);
		snapshot->surface_count = read_u16(bytes + 32);
		snapshot->resize_boundary_count = read_u16(bytes + 34);
		snapshot->has_focused_surface = (bytes[36] & 1u) != 0;
		snapshot->focused_surface_id = read_u64(bytes + 40);
		if (!scene_header_fields_valid(snapshot) || scene_message_size(snapshot) != size) {
			memset(message_out, 0, sizeof(*message_out));
			return CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS;
		}
		for (uint16_t index = 0; index < snapshot->surface_count; index++) {
			struct cg_scene_surface_state *state = &snapshot->surfaces[index];
			uint8_t flags = bytes[offset + 24];
			if ((flags & ~0x1fu) != 0 || !bytes_are_zero(bytes + offset + 25, 3) ||
			    !bytes_are_zero(bytes + offset + 56, 8)) {
				memset(message_out, 0, sizeof(*message_out));
				return CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED;
			}
			state->surface_id = read_u64(bytes + offset);
			state->bounds = (struct cg_scene_rect) {
				.x = read_i32(bytes + offset + 8),
				.y = read_i32(bytes + offset + 12),
				.width = read_i32(bytes + offset + 16),
				.height = read_i32(bytes + offset + 20),
			};
			state->has_clip = (flags & 1u) != 0;
			state->visible = (flags & 2u) != 0;
			state->accepts_input = (flags & 4u) != 0;
			state->has_parent = (flags & 8u) != 0;
			state->modal = (flags & 16u) != 0;
			state->z_index = read_i32(bytes + offset + 28);
			state->clip = (struct cg_scene_rect) {
				.x = read_i32(bytes + offset + 32),
				.y = read_i32(bytes + offset + 36),
				.width = read_i32(bytes + offset + 40),
				.height = read_i32(bytes + offset + 44),
			};
			state->parent_surface_id = read_u64(bytes + offset + 48);
			offset += CG_SURFACE_CONTROL_SURFACE_STATE_SIZE;
		}
		for (uint16_t index = 0; index < snapshot->resize_boundary_count; index++) {
			struct cg_scene_resize_boundary *boundary = &snapshot->resize_boundaries[index];
			if ((bytes[offset + 18] & ~3u) != 0 || bytes[offset + 19] != 0 ||
			    !bytes_are_zero(bytes + offset + 32, 8)) {
				memset(message_out, 0, sizeof(*message_out));
				return CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED;
			}
			boundary->boundary_id = read_u64(bytes + offset);
			boundary->target_surface_id = read_u64(bytes + offset + 8);
			boundary->edge = (enum cg_scene_resize_edge) bytes[offset + 16];
			boundary->cursor = (enum cg_scene_resize_cursor) bytes[offset + 17];
			boundary->enabled = (bytes[offset + 18] & 1u) != 0;
			boundary->visible = (bytes[offset + 18] & 2u) != 0;
			boundary->minimum_size = read_u32(bytes + offset + 20);
			boundary->maximum_size = read_u32(bytes + offset + 24);
			boundary->hit_slop = read_u32(bytes + offset + 28);
			offset += CG_SURFACE_CONTROL_RESIZE_BOUNDARY_SIZE;
		}
		return CG_SURFACE_CONTROL_PARSE_OK;
	}
	case CG_SURFACE_CONTROL_RESIZE_OUTPUT:
		if (size != CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE) {
			return CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
		}
		message_out->type = CG_SURFACE_CONTROL_RESIZE_OUTPUT;
		message_out->resize_output.scene_id = read_u64(bytes + 8);
		message_out->resize_output.output_id = read_u64(bytes + 16);
		message_out->resize_output.output_width = read_u32(bytes + 24);
		message_out->resize_output.output_height = read_u32(bytes + 28);
		if (message_out->resize_output.scene_id == 0 || message_out->resize_output.output_id == 0 ||
		    message_out->resize_output.output_width == 0 || message_out->resize_output.output_height == 0) {
			memset(message_out, 0, sizeof(*message_out));
			return CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS;
		}
		return CG_SURFACE_CONTROL_PARSE_OK;
	default:
		return CG_SURFACE_CONTROL_PARSE_UNKNOWN_TYPE;
	}
}

bool
cg_surface_control_encode_create_scene(const struct cg_surface_control_create_scene *request, uint8_t *bytes_out,
				       size_t capacity, size_t *size_out)
{
	if (!request || request->scene_id == 0 || request->output_id == 0 || request->output_width == 0 ||
	    request->output_height == 0 || !bytes_out || capacity < CG_SURFACE_CONTROL_CREATE_SCENE_SIZE || !size_out) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_CREATE_SCENE_SIZE);
	write_header(bytes_out, CG_SURFACE_CONTROL_CREATE_SCENE, CG_SURFACE_CONTROL_CREATE_SCENE_SIZE);
	write_u64(bytes_out + 8, request->scene_id);
	write_u64(bytes_out + 16, request->output_id);
	write_u32(bytes_out + 24, request->output_width);
	write_u32(bytes_out + 28, request->output_height);
	*size_out = CG_SURFACE_CONTROL_CREATE_SCENE_SIZE;
	return true;
}

bool
cg_surface_control_encode_destroy_scene(const struct cg_surface_control_destroy_scene *request, uint8_t *bytes_out,
					size_t capacity, size_t *size_out)
{
	if (!request || request->scene_id == 0 || !bytes_out || capacity < CG_SURFACE_CONTROL_DESTROY_SCENE_SIZE ||
	    !size_out) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_DESTROY_SCENE_SIZE);
	write_header(bytes_out, CG_SURFACE_CONTROL_DESTROY_SCENE, CG_SURFACE_CONTROL_DESTROY_SCENE_SIZE);
	write_u64(bytes_out + 8, request->scene_id);
	*size_out = CG_SURFACE_CONTROL_DESTROY_SCENE_SIZE;
	return true;
}

bool
cg_surface_control_encode_apply_scene(const struct cg_scene_snapshot *snapshot, uint8_t *bytes_out, size_t capacity,
				      size_t *size_out)
{
	size_t size;
	size_t offset = CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE;

	if (!scene_header_fields_valid(snapshot) || !bytes_out || !size_out) {
		return false;
	}
	size = scene_message_size(snapshot);
	if (capacity < size || size > UINT16_MAX) {
		return false;
	}
	memset(bytes_out, 0, size);
	write_header(bytes_out, CG_SURFACE_CONTROL_APPLY_SCENE, (uint16_t) size);
	write_u64(bytes_out + 8, snapshot->scene_id);
	write_u64(bytes_out + 16, snapshot->output_id);
	write_u64(bytes_out + 24, snapshot->revision);
	write_u16(bytes_out + 32, snapshot->surface_count);
	write_u16(bytes_out + 34, snapshot->resize_boundary_count);
	bytes_out[36] = snapshot->has_focused_surface ? 1 : 0;
	write_u64(bytes_out + 40, snapshot->focused_surface_id);
	for (uint16_t index = 0; index < snapshot->surface_count; index++) {
		const struct cg_scene_surface_state *state = &snapshot->surfaces[index];
		uint8_t flags = (state->has_clip ? 1u : 0u) | (state->visible ? 2u : 0u) |
				(state->accepts_input ? 4u : 0u) | (state->has_parent ? 8u : 0u) |
				(state->modal ? 16u : 0u);
		write_u64(bytes_out + offset, state->surface_id);
		write_i32(bytes_out + offset + 8, state->bounds.x);
		write_i32(bytes_out + offset + 12, state->bounds.y);
		write_i32(bytes_out + offset + 16, state->bounds.width);
		write_i32(bytes_out + offset + 20, state->bounds.height);
		bytes_out[offset + 24] = flags;
		write_i32(bytes_out + offset + 28, state->z_index);
		write_i32(bytes_out + offset + 32, state->clip.x);
		write_i32(bytes_out + offset + 36, state->clip.y);
		write_i32(bytes_out + offset + 40, state->clip.width);
		write_i32(bytes_out + offset + 44, state->clip.height);
		write_u64(bytes_out + offset + 48, state->parent_surface_id);
		offset += CG_SURFACE_CONTROL_SURFACE_STATE_SIZE;
	}
	for (uint16_t index = 0; index < snapshot->resize_boundary_count; index++) {
		const struct cg_scene_resize_boundary *boundary = &snapshot->resize_boundaries[index];
		write_u64(bytes_out + offset, boundary->boundary_id);
		write_u64(bytes_out + offset + 8, boundary->target_surface_id);
		bytes_out[offset + 16] = (uint8_t) boundary->edge;
		bytes_out[offset + 17] = (uint8_t) boundary->cursor;
		bytes_out[offset + 18] = (boundary->enabled ? 1u : 0u) | (boundary->visible ? 2u : 0u);
		write_u32(bytes_out + offset + 20, boundary->minimum_size);
		write_u32(bytes_out + offset + 24, boundary->maximum_size);
		write_u32(bytes_out + offset + 28, boundary->hit_slop);
		offset += CG_SURFACE_CONTROL_RESIZE_BOUNDARY_SIZE;
	}
	*size_out = size;
	return true;
}

bool
cg_surface_control_encode_resize_output(const struct cg_surface_control_resize_output *request, uint8_t *bytes_out,
					size_t capacity, size_t *size_out)
{
	if (!request || request->scene_id == 0 || request->output_id == 0 || request->output_width == 0 ||
	    request->output_height == 0 || !bytes_out || capacity < CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE ||
	    !size_out) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE);
	write_header(bytes_out, CG_SURFACE_CONTROL_RESIZE_OUTPUT, CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE);
	write_u64(bytes_out + 8, request->scene_id);
	write_u64(bytes_out + 16, request->output_id);
	write_u32(bytes_out + 24, request->output_width);
	write_u32(bytes_out + 28, request->output_height);
	*size_out = CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE;
	return true;
}

bool
cg_surface_control_encode_output_changed(const struct cg_surface_control_resize_output *event, uint8_t *bytes_out,
					 size_t capacity, size_t *size_out)
{
	if (!cg_surface_control_encode_resize_output(event, bytes_out, capacity, size_out)) {
		return false;
	}
	bytes_out[5] = CG_SURFACE_CONTROL_OUTPUT_CHANGED;
	return true;
}

bool
cg_surface_control_encode_resize_event(const struct cg_resize_event *event, uint8_t *bytes_out, size_t capacity,
				       size_t *size_out)
{
	enum cg_surface_control_message_type type = CG_SURFACE_CONTROL_BOUNDS_CHANGING;

	if (!event || event->scene_id == 0 || event->revision == 0 || event->boundary_id == 0 ||
	    event->surface_id == 0 || event->bounds.width <= 0 || event->bounds.height <= 0 || !bytes_out ||
	    capacity < CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE || !size_out) {
		return false;
	}
	switch (event->type) {
	case CG_RESIZE_EVENT_BOUNDS_CHANGING:
		type = CG_SURFACE_CONTROL_BOUNDS_CHANGING;
		break;
	case CG_RESIZE_EVENT_BOUNDS_COMMITTED:
		type = CG_SURFACE_CONTROL_BOUNDS_COMMITTED;
		break;
	case CG_RESIZE_EVENT_CANCELLED:
		type = CG_SURFACE_CONTROL_RESIZE_CANCELLED;
		break;
	case CG_RESIZE_EVENT_NONE:
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE);
	write_header(bytes_out, type, CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE);
	write_u64(bytes_out + 8, event->scene_id);
	write_u64(bytes_out + 16, event->revision);
	write_u64(bytes_out + 24, event->boundary_id);
	write_u64(bytes_out + 32, event->surface_id);
	write_i32(bytes_out + 40, event->bounds.x);
	write_i32(bytes_out + 44, event->bounds.y);
	write_i32(bytes_out + 48, event->bounds.width);
	write_i32(bytes_out + 52, event->bounds.height);
	*size_out = CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE;
	return true;
}

bool
cg_surface_control_encode_register(const struct cg_surface_registration_request *request, uint8_t *bytes_out,
				   size_t capacity, size_t *size_out)
{
	if (!registration_fields_valid(request) || !bytes_out || capacity < CG_SURFACE_CONTROL_REGISTER_SIZE ||
	    !size_out) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_REGISTER_SIZE);
	write_header(bytes_out, CG_SURFACE_CONTROL_REGISTER, CG_SURFACE_CONTROL_REGISTER_SIZE);
	write_u64(bytes_out + 8, request->scene_id);
	write_u64(bytes_out + 16, request->surface_id);
	bytes_out[24] = (uint8_t) request->kind;
	bytes_out[25] = request->has_parent ? 1 : 0;
	write_u64(bytes_out + 28, request->parent_surface_id);
	write_u32(bytes_out + 36, request->association_timeout_ms);
	memcpy(bytes_out + 40, request->token.bytes, CG_SURFACE_TOKEN_SIZE);
	*size_out = CG_SURFACE_CONTROL_REGISTER_SIZE;
	return true;
}

bool
cg_surface_control_encode_unregister(const struct cg_surface_control_unregister *request, uint8_t *bytes_out,
				     size_t capacity, size_t *size_out)
{
	if (!request || request->scene_id == 0 || request->surface_id == 0 || !bytes_out ||
	    capacity < CG_SURFACE_CONTROL_UNREGISTER_SIZE || !size_out) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_UNREGISTER_SIZE);
	write_header(bytes_out, CG_SURFACE_CONTROL_UNREGISTER, CG_SURFACE_CONTROL_UNREGISTER_SIZE);
	write_u64(bytes_out + 8, request->scene_id);
	write_u64(bytes_out + 16, request->surface_id);
	*size_out = CG_SURFACE_CONTROL_UNREGISTER_SIZE;
	return true;
}

bool
cg_surface_control_encode_reset(uint8_t *bytes_out, size_t capacity, size_t *size_out)
{
	if (!bytes_out || capacity < CG_SURFACE_CONTROL_RESET_SIZE || !size_out) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_RESET_SIZE);
	write_header(bytes_out, CG_SURFACE_CONTROL_RESET, CG_SURFACE_CONTROL_RESET_SIZE);
	*size_out = CG_SURFACE_CONTROL_RESET_SIZE;
	return true;
}

static bool
encode_identity_event(enum cg_surface_control_message_type type, cg_scene_id scene_id, cg_surface_id surface_id,
		      enum cg_surface_kind kind, bool has_parent, cg_surface_id parent_surface_id, uint8_t *bytes_out,
		      size_t capacity, size_t *size_out)
{
	if (scene_id == 0 || surface_id == 0 || !cg_surface_kind_is_valid(kind) || !bytes_out ||
	    capacity < CG_SURFACE_CONTROL_REGISTERED_SIZE || !size_out ||
	    (has_parent ? parent_surface_id == 0 || parent_surface_id == surface_id : parent_surface_id != 0) ||
	    (kind == CG_SURFACE_KIND_POPUP && !has_parent)) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_REGISTERED_SIZE);
	write_header(bytes_out, type, CG_SURFACE_CONTROL_REGISTERED_SIZE);
	write_u64(bytes_out + 8, scene_id);
	write_u64(bytes_out + 16, surface_id);
	bytes_out[24] = (uint8_t) kind;
	bytes_out[25] = has_parent ? 1 : 0;
	write_u64(bytes_out + 28, parent_surface_id);
	*size_out = CG_SURFACE_CONTROL_REGISTERED_SIZE;
	return true;
}

bool
cg_surface_control_encode_registered(const struct cg_surface_registration_request *request, uint8_t *bytes_out,
				     size_t capacity, size_t *size_out)
{
	if (!registration_fields_valid(request)) {
		return false;
	}
	return encode_identity_event(CG_SURFACE_CONTROL_REGISTERED, request->scene_id, request->surface_id,
				     request->kind, request->has_parent, request->parent_surface_id, bytes_out,
				     capacity, size_out);
}

bool
cg_surface_control_encode_associated(const struct cg_surface_identity *identity, uint8_t *bytes_out, size_t capacity,
				     size_t *size_out)
{
	if (!identity) {
		return false;
	}
	return encode_identity_event(CG_SURFACE_CONTROL_ASSOCIATED, identity->scene_id, identity->surface_id,
				     identity->kind, identity->has_parent, identity->parent_surface_id, bytes_out,
				     capacity, size_out);
}
