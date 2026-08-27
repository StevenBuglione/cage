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
test_first_frame_golden_vector(void)
{
	const struct cg_surface_control_first_frame event = {
		.scene_id = 0x0102030405060708ULL,
		.surface_id = 0x1112131415161718ULL,
		.revision = 0x2122232425262728ULL,
		.width = 1920,
		.height = 1080,
	};
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size = 0;

	assert(cg_surface_control_encode_first_frame(&event, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_FIRST_FRAME_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x86\x00\x28", 8) == 0);
	assert(memcmp(bytes + 8, "\x01\x02\x03\x04\x05\x06\x07\x08", 8) == 0);
	assert(memcmp(bytes + 16, "\x11\x12\x13\x14\x15\x16\x17\x18", 8) == 0);
	assert(memcmp(bytes + 24, "\x21\x22\x23\x24\x25\x26\x27\x28", 8) == 0);
	assert(bytes[32] == 0 && bytes[33] == 0 && bytes[34] == 7 && bytes[35] == 128);
	assert(bytes[36] == 0 && bytes[37] == 0 && bytes[38] == 4 && bytes[39] == 56);
}

static void
test_presentation_pending_golden_vector(void)
{
	const struct cg_surface_control_presentation_pending event = {
		.scene_id = 0x0102030405060708ULL,
		.surface_id = 0x1112131415161718ULL,
		.revision = 0x2122232425262728ULL,
		.actual_width = 1280,
		.actual_height = 720,
		.expected_width = 1440,
		.expected_height = 900,
		.reason = CG_SURFACE_CONTROL_PRESENTATION_PENDING_BUFFER_SIZE_MISMATCH,
	};
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size = 0;

	assert(cg_surface_control_encode_presentation_pending(&event, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_PRESENTATION_PENDING_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x87\x00\x38", 8) == 0);
	assert(memcmp(bytes + 8, "\x01\x02\x03\x04\x05\x06\x07\x08", 8) == 0);
	assert(memcmp(bytes + 16, "\x11\x12\x13\x14\x15\x16\x17\x18", 8) == 0);
	assert(memcmp(bytes + 24, "\x21\x22\x23\x24\x25\x26\x27\x28", 8) == 0);
	assert(bytes[34] == 5 && bytes[35] == 0);
	assert(bytes[38] == 2 && bytes[39] == 208);
	assert(bytes[42] == 5 && bytes[43] == 160);
	assert(bytes[46] == 3 && bytes[47] == 132);
	assert(bytes[51] == CG_SURFACE_CONTROL_PRESENTATION_PENDING_BUFFER_SIZE_MISMATCH);
	assert(bytes[52] == 0 && bytes[53] == 0 && bytes[54] == 0 && bytes[55] == 0);
}

static void
test_registered_golden_vector(void)
{
	struct cg_surface_registration_request request = registration();
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size = 0;

	assert(cg_surface_control_encode_registered(&request, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_REGISTERED_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x80\x00\x28", 8) == 0);
	assert(memcmp(bytes + 8, "\x01\x02\x03\x04\x05\x06\x07\x08", 8) == 0);
	assert(memcmp(bytes + 16, "\x11\x12\x13\x14\x15\x16\x17\x18", 8) == 0);
	assert(bytes[24] == CG_SURFACE_KIND_POPUP && bytes[25] == 1);
	assert(memcmp(bytes + 28, "!\x22#$%&'(\x00\x00\x00\x00", 12) == 0);
}

static struct cg_scene_snapshot
scene_snapshot(void)
{
	struct cg_scene_snapshot snapshot = {
		.scene_id = 0x0102030405060708ULL,
		.output_id = 0x1112131415161718ULL,
		.revision = 0x2122232425262728ULL,
		.snapshot_output_width = 1440,
		.snapshot_output_height = 900,
		.surface_count = 2,
		.resize_boundary_count = 1,
		.has_focused_surface = true,
		.focused_surface_id = 200,
	};
	snapshot.surfaces[0] = (struct cg_scene_surface_state) {
		.surface_id = 100,
		.bounds = {.x = -10, .y = 20, .width = 800, .height = 600},
		.z_index = -5,
		.visible = true,
	};
	snapshot.surfaces[1] = (struct cg_scene_surface_state) {
		.surface_id = 200,
		.bounds = {.x = 400, .y = 40, .width = 600, .height = 560},
		.has_clip = true,
		.clip = {.x = 450, .y = 50, .width = 500, .height = 500},
		.z_index = 10,
		.visible = true,
		.accepts_input = true,
		.has_parent = true,
		.parent_surface_id = 100,
		.modal = true,
		.output_anchor_mask = CG_SCENE_OUTPUT_ANCHOR_MASK,
	};
	snapshot.resize_boundaries[0] = (struct cg_scene_resize_boundary) {
		.boundary_id = 55,
		.target_surface_id = 200,
		.edge = CG_SCENE_RESIZE_EDGE_LEFT,
		.minimum_size = 360,
		.maximum_size = 1200,
		.hit_slop = 10,
		.cursor = CG_SCENE_RESIZE_CURSOR_COLUMN,
		.enabled = true,
		.visible = true,
	};
	return snapshot;
}

static void
test_scene_control_round_trip(void)
{
	const struct cg_surface_control_create_scene create = {
		.scene_id = 7,
		.output_id = 11,
		.output_width = 1440,
		.output_height = 900,
	};
	const struct cg_surface_control_destroy_scene destroy = {.scene_id = 7};
	const struct cg_surface_control_resize_output resize = {
		.scene_id = 7,
		.output_id = 11,
		.output_width = 1920,
		.output_height = 1080,
	};
	struct cg_scene_snapshot snapshot = scene_snapshot();
	struct cg_surface_control_message parsed;
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	size_t size;

	assert(cg_surface_control_encode_create_scene(&create, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_CREATE_SCENE_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x04\x00\x20", 8) == 0);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	assert(parsed.type == CG_SURFACE_CONTROL_CREATE_SCENE);
	assert(parsed.create_scene.scene_id == 7 && parsed.create_scene.output_id == 11);
	assert(parsed.create_scene.output_width == 1440 && parsed.create_scene.output_height == 900);

	assert(cg_surface_control_encode_destroy_scene(&destroy, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_DESTROY_SCENE_SIZE);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	assert(parsed.type == CG_SURFACE_CONTROL_DESTROY_SCENE && parsed.destroy_scene.scene_id == 7);

	assert(cg_surface_control_encode_resize_output(&resize, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	assert(parsed.type == CG_SURFACE_CONTROL_RESIZE_OUTPUT);
	assert(parsed.resize_output.output_id == 11 && parsed.resize_output.output_width == 1920);
	assert(cg_surface_control_encode_output_changed(&resize, bytes, sizeof(bytes), &size));
	assert(memcmp(bytes, "LSC1\x01\x85\x00\x20", 8) == 0);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_UNKNOWN_TYPE);

	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE + 2 * CG_SURFACE_CONTROL_SURFACE_STATE_SIZE +
			       CG_SURFACE_CONTROL_RESIZE_BOUNDARY_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x06\x00\xe0", 8) == 0);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	assert(parsed.type == CG_SURFACE_CONTROL_APPLY_SCENE);
	assert(parsed.scene_snapshot.scene_id == snapshot.scene_id);
	assert(parsed.scene_snapshot.output_id == snapshot.output_id);
	assert(parsed.scene_snapshot.revision == snapshot.revision);
	assert(parsed.scene_snapshot.snapshot_output_width == 1440);
	assert(parsed.scene_snapshot.snapshot_output_height == 900);
	assert(parsed.scene_snapshot.surface_count == 2);
	assert(parsed.scene_snapshot.resize_boundary_count == 1);
	assert(parsed.scene_snapshot.surfaces[0].bounds.x == -10);
	assert(parsed.scene_snapshot.surfaces[0].z_index == -5);
	assert(parsed.scene_snapshot.surfaces[1].has_clip);
	assert(parsed.scene_snapshot.surfaces[1].clip.width == 500);
	assert(parsed.scene_snapshot.surfaces[1].modal);
	assert(parsed.scene_snapshot.surfaces[1].output_anchor_mask == CG_SCENE_OUTPUT_ANCHOR_MASK);
	assert(parsed.scene_snapshot.resize_boundaries[0].boundary_id == 55);
	assert(parsed.scene_snapshot.resize_boundaries[0].maximum_size == 1200);
	assert(parsed.scene_snapshot.resize_boundaries[0].visible);
}

static void
test_scene_message_rejection_and_capacity(void)
{
	struct cg_scene_snapshot snapshot = scene_snapshot();
	struct cg_surface_control_message parsed;
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE + 1];
	size_t size;

	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	for (size_t truncated = 0; truncated < size; truncated++) {
		assert(cg_surface_control_parse(bytes, truncated, &parsed) != CG_SURFACE_CONTROL_PARSE_OK);
	}
	bytes[37] = 1;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED);
	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	bytes[CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE + 26] = 1;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED);
	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	bytes[CG_SURFACE_CONTROL_APPLY_SCENE_HEADER_SIZE + 25] = 0x10;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED);
	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	bytes[size - 1] = 1;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_RESERVED);
	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	bytes[32] = 0xff;
	bytes[33] = 0xff;
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_FIELDS);

	memset(&snapshot, 0, sizeof(snapshot));
	snapshot.scene_id = 7;
	snapshot.output_id = 11;
	snapshot.revision = 1;
	snapshot.snapshot_output_width = 1440;
	snapshot.snapshot_output_height = 900;
	snapshot.surface_count = CG_SCENE_SURFACE_CAPACITY;
	snapshot.resize_boundary_count = CG_SCENE_RESIZE_BOUNDARY_CAPACITY;
	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_OK);
	bytes[size] = 0;
	assert(cg_surface_control_parse(bytes, size + 1, &parsed) == CG_SURFACE_CONTROL_PARSE_INVALID_SIZE);

	snapshot.scene_id = 0;
	assert(!cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	assert(!cg_surface_control_encode_create_scene(NULL, bytes, sizeof(bytes), &size));
	assert(!cg_surface_control_encode_destroy_scene(NULL, bytes, sizeof(bytes), &size));
	assert(!cg_surface_control_encode_resize_output(NULL, bytes, sizeof(bytes), &size));
}

static void
test_resize_event_golden_vector(void)
{
	struct cg_resize_event event = {
		.type = CG_RESIZE_EVENT_BOUNDS_COMMITTED,
		.scene_id = 7,
		.revision = 9,
		.boundary_id = 55,
		.surface_id = 200,
		.bounds = {.x = -10, .y = 20, .width = 640, .height = 480},
	};
	struct cg_surface_control_message parsed;
	uint8_t bytes[CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE];
	size_t size;

	assert(cg_surface_control_encode_resize_event(&event, bytes, sizeof(bytes), &size));
	assert(size == CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x83\x00\x38", 8) == 0);
	assert(memcmp(bytes + 32, "\x00\x00\x00\x00\x00\x00\x00\xc8", 8) == 0);
	assert(memcmp(bytes + 40, "\xff\xff\xff\xf6\x00\x00\x00\x14", 8) == 0);
	assert(cg_surface_control_parse(bytes, size, &parsed) == CG_SURFACE_CONTROL_PARSE_UNKNOWN_TYPE);
	event.type = CG_RESIZE_EVENT_NONE;
	assert(!cg_surface_control_encode_resize_event(&event, bytes, sizeof(bytes), &size));
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
	test_first_frame_golden_vector();
	test_presentation_pending_golden_vector();
	test_registered_golden_vector();
	test_scene_control_round_trip();
	test_scene_message_rejection_and_capacity();
	test_resize_event_golden_vector();
	test_size_and_header_rejection();
	test_register_field_rejection();
	test_encoder_rejection();
	return 0;
}
