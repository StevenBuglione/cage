#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "surface_control_protocol.h"

static struct cg_surface_registration_request
registration(void)
{
	struct cg_surface_registration_request request = {
		.scene_id = 0x0102030405060708ULL,
		.surface_id = 0x1112131415161718ULL,
		.kind = CG_SURFACE_KIND_POPUP,
		.has_parent = true,
		.parent_surface_id = 0x2122232425262728ULL,
		.association_timeout_ms = 5000,
	};
	for (size_t index = 0; index < CG_SURFACE_TOKEN_SIZE; index++) {
		request.token.bytes[index] = (uint8_t) index + 1;
	}
	return request;
}

static void
test_register_golden_vector(void)
{
	struct cg_surface_registration_request request = registration();
	struct cg_surface_control_message parsed;
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size = 0;

	assert(cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_REGISTER_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x01\x00\x48", 8) == 0);
	assert(memcmp(bytes + 8, "\x01\x02\x03\x04\x05\x06\x07\x08", 8) == 0);
	assert(memcmp(bytes + 16, "\x11\x12\x13\x14\x15\x16\x17\x18", 8) == 0);
	assert(bytes[24] == CG_SURFACE_KIND_POPUP);
	assert(bytes[25] == 1);
	assert(bytes[26] == 0 && bytes[27] == 0);
	assert(memcmp(bytes + 28, "!\x22#$%&'(\x00\x00\x13\x88", 12) == 0);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	assert(parsed.type == CG_SURFACE_CONTROL_REGISTER);
	assert(parsed.registration.scene_id == request.scene_id);
	assert(parsed.registration.surface_id == request.surface_id);
	assert(parsed.registration.kind == request.kind);
	assert(parsed.registration.has_parent);
	assert(parsed.registration.parent_surface_id == request.parent_surface_id);
	assert(parsed.registration.association_timeout_ms == 5000);
	assert(memcmp(&parsed.registration.token, &request.token, sizeof(request.token)) == 0);
}

static void
test_unregister_and_reset(void)
{
	struct cg_surface_control_unregister unregister_request = {.scene_id = 7, .surface_id = 100};
	struct cg_surface_control_message parsed;
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size;

	assert(cg_surface_control_encode_unregister(&unregister_request, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_UNREGISTER_SIZE);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	assert(parsed.type == CG_SURFACE_CONTROL_UNREGISTER);
	assert(parsed.unregistration.scene_id == 7);
	assert(parsed.unregistration.surface_id == 100);

	assert(cg_surface_control_encode_reset(bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_RESET_SIZE);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	assert(parsed.type == CG_SURFACE_CONTROL_RESET);
}

static void
test_associated_golden_vector(void)
{
	const struct cg_surface_identity identity = {
		.scene_id = 0x0102030405060708ULL,
		.surface_id = 0x1112131415161718ULL,
		.kind = CG_SURFACE_KIND_POPUP,
		.has_parent = true,
		.parent_surface_id = 0x2122232425262728ULL,
	};
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	struct cg_surface_control_message parsed;
	size_t size;

	assert(cg_surface_control_encode_associated(&identity, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_ASSOCIATED_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x81\x00\x28", 8) == 0);
	assert(memcmp(bytes + 8, "\x01\x02\x03\x04\x05\x06\x07\x08", 8) == 0);
	assert(memcmp(bytes + 16, "\x11\x12\x13\x14\x15\x16\x17\x18", 8) == 0);
	assert(bytes[24] == CG_SURFACE_KIND_POPUP);
	assert(bytes[25] == 1);
	assert(bytes[26] == 0 && bytes[27] == 0);
	assert(memcmp(bytes + 28, "!\x22#$%&'(\x00\x00\x00\x00", 12) == 0);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_UNKNOWN_TYPE);
}

static void
test_size_and_header_rejection(void)
{
	struct cg_surface_registration_request request = registration();
	struct cg_surface_control_message parsed;
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE + 1];
	size_t size;

	assert(cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	for (size_t truncated = 0; truncated < size; truncated++) {
		assert(cg_surface_control_parse(bytes, truncated, &parsed) != CG_SURFACE_CONTROL_PARSE_OK);
	}
	bytes[size] = 0;
	assert(cg_surface_control_parse(bytes, size + 1, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_SIZE);
	assert(cg_surface_control_parse(NULL, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_SIZE);
	assert(cg_surface_control_parse(bytes, size, NULL) == CG_SURFACE_CONTROL_PARSE_INVALID_SIZE);

	bytes[0] = 'X';
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_MAGIC);
	bytes[0] = 'L';
	bytes[4] = 2;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_UNSUPPORTED_VERSION);
	bytes[4] = CG_SURFACE_CONTROL_VERSION;
	bytes[5] = 99;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_UNKNOWN_TYPE);
	bytes[5] = CG_SURFACE_CONTROL_REGISTER;
	bytes[7] = 71;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_SIZE);
}

static void
test_register_field_rejection(void)
{
	struct cg_surface_registration_request request = registration();
	struct cg_surface_control_message parsed;
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size;

	assert(cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	bytes[26] = 1;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED);
	bytes[26] = 0;
	bytes[25] = 2;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED);
	bytes[25] = 1;
	memset(bytes + 8, 0, 8);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS);
	assert(cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	bytes[24] = 0;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS);
	assert(cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	memset(bytes + 40, 0, CG_SURFACE_TOKEN_SIZE);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS);
	assert(cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	memset(bytes + 36, 0, 4);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS);
	assert(cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	bytes[25] = 0;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS);
}

static void
test_encoder_rejection(void)
{
	struct cg_surface_registration_request request = registration();
	struct cg_surface_control_unregister unregister_request = {0};
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size;

	assert(!cg_surface_control_encode_register(NULL, bytes, sizeof(bytes), &size));
	assert(!cg_surface_control_encode_register(&request, NULL, sizeof(bytes), &size));
	assert(!cg_surface_control_encode_register(&request, bytes, CG_SURFACE_CONTROL_REGISTER_SIZE - 1, &size));
	assert(!cg_surface_control_encode_register(&request, bytes, sizeof(bytes), NULL));
	request.kind = (enum cg_surface_kind) 99;
	assert(!cg_surface_control_encode_register(&request, bytes, sizeof(bytes), &size));
	assert(!cg_surface_control_encode_unregister(&unregister_request, bytes, sizeof(bytes), &size));
	assert(!cg_surface_control_encode_reset(bytes, CG_SURFACE_CONTROL_RESET_SIZE - 1, &size));
	assert(!cg_surface_control_encode_associated(NULL, bytes, sizeof(bytes), &size));
}

int
main(void)
{
	test_register_golden_vector();
	test_unregister_and_reset();
	test_associated_golden_vector();
	test_size_and_header_rejection();
	test_register_field_rejection();
	test_encoder_rejection();
	return 0;
}
