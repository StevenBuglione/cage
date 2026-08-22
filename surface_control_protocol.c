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
	default:
		return CG_SURFACE_CONTROL_PARSE_UNKNOWN_TYPE;
	}
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

bool
cg_surface_control_encode_associated(const struct cg_surface_identity *identity, uint8_t *bytes_out,
				     size_t capacity, size_t *size_out)
{
	if (!identity || identity->scene_id == 0 || identity->surface_id == 0 ||
	    !cg_surface_kind_is_valid(identity->kind) || !bytes_out || capacity < CG_SURFACE_CONTROL_ASSOCIATED_SIZE ||
	    !size_out ||
	    (identity->has_parent ? identity->parent_surface_id == 0 ||
				     identity->parent_surface_id == identity->surface_id
				   : identity->parent_surface_id != 0) ||
	    (identity->kind == CG_SURFACE_KIND_POPUP && !identity->has_parent)) {
		return false;
	}
	memset(bytes_out, 0, CG_SURFACE_CONTROL_ASSOCIATED_SIZE);
	write_header(bytes_out, CG_SURFACE_CONTROL_ASSOCIATED, CG_SURFACE_CONTROL_ASSOCIATED_SIZE);
	write_u64(bytes_out + 8, identity->scene_id);
	write_u64(bytes_out + 16, identity->surface_id);
	bytes_out[24] = (uint8_t) identity->kind;
	bytes_out[25] = identity->has_parent ? 1 : 0;
	write_u64(bytes_out + 28, identity->parent_surface_id);
	*size_out = CG_SURFACE_CONTROL_ASSOCIATED_SIZE;
	return true;
}
