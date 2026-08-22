#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "surface_registry.h"
#include "surface_token_hint.h"

static struct cg_surface_token
token_for(uint64_t value)
{
	struct cg_surface_token token = {0};

	for (size_t index = 0; index < sizeof(value); index++) {
		token.bytes[index] = (uint8_t) (value >> (index * 8));
	}
	token.bytes[CG_SURFACE_TOKEN_SIZE - 1] = 0xa5;
	return token;
}

static struct cg_surface_registration_request
request_for(cg_surface_id surface_id, uint64_t token_value, enum cg_surface_kind kind)
{
	struct cg_surface_registration_request request = {
		.scene_id = 7,
		.surface_id = surface_id,
		.token = token_for(token_value),
		.kind = kind,
		.association_timeout_ms = 5000,
	};
	return request;
}

static void
test_registration_and_immutable_association(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_registration_request request = request_for(100, 1, CG_SURFACE_KIND_APP_VIEW);
	struct cg_surface_identity identity;

	cg_surface_registry_init(&registry);
	assert(cg_surface_registry_register(&registry, &request, 1000) == CG_SURFACE_REGISTRY_OK);
	assert(registry.registrations == 1);
	assert(cg_surface_registry_associate(&registry, &request.token, 0x1000, 2000, &identity) ==
	       CG_SURFACE_REGISTRY_OK);
	assert(identity.scene_id == 7);
	assert(identity.surface_id == 100);
	assert(identity.kind == CG_SURFACE_KIND_APP_VIEW);
	assert(!identity.has_parent);
	assert(registry.associations == 1);
	assert(cg_surface_registry_find(&registry, 7, 100)->state == CG_SURFACE_REGISTRATION_ASSOCIATED);
	assert(cg_surface_registry_find(&registry, 7, 100)->associated_surface == 0x1000);
	assert(cg_surface_registry_associate(&registry, &request.token, 0x2000, 3000, &identity) ==
	       CG_SURFACE_REGISTRY_TOKEN_REPLAYED);
	assert(cg_surface_registry_find(&registry, 7, 100)->associated_surface == 0x1000);
}

static void
test_invalid_and_duplicate_registration(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_registration_request first = request_for(100, 1, CG_SURFACE_KIND_APP_VIEW);
	struct cg_surface_registration_request duplicate_id = request_for(100, 2, CG_SURFACE_KIND_FIREFOX_VIEW);
	struct cg_surface_registration_request duplicate_token = request_for(101, 1, CG_SURFACE_KIND_FIREFOX_VIEW);
	struct cg_surface_registration_request invalid = request_for(102, 3, (enum cg_surface_kind) 99);

	cg_surface_registry_init(&registry);
	assert(cg_surface_registry_register(&registry, &first, 0) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_register(&registry, &duplicate_id, 0) == CG_SURFACE_REGISTRY_DUPLICATE_ID);
	assert(cg_surface_registry_register(&registry, &duplicate_token, 0) == CG_SURFACE_REGISTRY_DUPLICATE_TOKEN);
	assert(cg_surface_registry_register(&registry, &invalid, 0) == CG_SURFACE_REGISTRY_INVALID);
	invalid = request_for(102, 3, CG_SURFACE_KIND_FIREFOX_VIEW);
	invalid.scene_id = 0;
	assert(cg_surface_registry_register(&registry, &invalid, 0) == CG_SURFACE_REGISTRY_INVALID);
	invalid = request_for(102, 3, CG_SURFACE_KIND_FIREFOX_VIEW);
	invalid.token = (struct cg_surface_token) {0};
	assert(cg_surface_registry_register(&registry, &invalid, 0) == CG_SURFACE_REGISTRY_INVALID);
	invalid = request_for(102, 3, CG_SURFACE_KIND_FIREFOX_VIEW);
	invalid.association_timeout_ms = 0;
	assert(cg_surface_registry_register(&registry, &invalid, 0) == CG_SURFACE_REGISTRY_INVALID);
	invalid.association_timeout_ms = CG_SURFACE_ASSOCIATION_TIMEOUT_MAX_MS + 1;
	assert(cg_surface_registry_register(&registry, &invalid, 0) == CG_SURFACE_REGISTRY_INVALID);
	invalid = request_for(102, 3, CG_SURFACE_KIND_FIREFOX_VIEW);
	assert(cg_surface_registry_register(&registry, &invalid, UINT64_MAX) == CG_SURFACE_REGISTRY_INVALID);
	assert(registry.registrations == 1);
	assert(registry.rejected_requests == 8);
}

static void
test_parent_rules(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_registration_request parent = request_for(100, 1, CG_SURFACE_KIND_APP_VIEW);
	struct cg_surface_registration_request popup = request_for(101, 2, CG_SURFACE_KIND_POPUP);
	struct cg_surface_registration_request overlay = request_for(102, 3, CG_SURFACE_KIND_OVERLAY);

	cg_surface_registry_init(&registry);
	assert(cg_surface_registry_register(&registry, &popup, 0) == CG_SURFACE_REGISTRY_PARENT_NOT_FOUND);
	popup.has_parent = true;
	popup.parent_surface_id = 999;
	assert(cg_surface_registry_register(&registry, &popup, 0) == CG_SURFACE_REGISTRY_PARENT_NOT_FOUND);
	assert(cg_surface_registry_register(&registry, &parent, 0) == CG_SURFACE_REGISTRY_OK);
	popup.parent_surface_id = 100;
	assert(cg_surface_registry_register(&registry, &popup, 0) == CG_SURFACE_REGISTRY_OK);
	overlay.has_parent = true;
	overlay.parent_surface_id = 100;
	assert(cg_surface_registry_register(&registry, &overlay, 0) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_find(&registry, 7, 101)->identity.parent_surface_id == 100);
}

static void
test_expiry_unknown_surface_and_retirement(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_registration_request first = request_for(100, 1, CG_SURFACE_KIND_FIREFOX_VIEW);
	struct cg_surface_registration_request second = request_for(101, 2, CG_SURFACE_KIND_OVERLAY);
	struct cg_surface_identity identity;
	struct cg_surface_token unknown = token_for(999);

	cg_surface_registry_init(&registry);
	first.association_timeout_ms = 10;
	assert(cg_surface_registry_register(&registry, &first, 100) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_register(&registry, &second, 100) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_associate(&registry, &unknown, 0x3000, 105, &identity) ==
	       CG_SURFACE_REGISTRY_UNKNOWN_TOKEN);
	assert(cg_surface_registry_associate(&registry, &first.token, 0x3000, 110, &identity) ==
	       CG_SURFACE_REGISTRY_TOKEN_EXPIRED);
	assert(registry.expirations == 1);
	assert(cg_surface_registry_associate(&registry, &second.token, 0x3000, 110, &identity) ==
	       CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_retire(&registry, 7, 101) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_associate(&registry, &second.token, 0x4000, 120, &identity) ==
	       CG_SURFACE_REGISTRY_TOKEN_REPLAYED);
	assert(cg_surface_registry_retire(&registry, 7, 101) == CG_SURFACE_REGISTRY_NOT_FOUND);
	assert(cg_surface_registry_retire(&registry, 7, 999) == CG_SURFACE_REGISTRY_NOT_FOUND);
	assert(registry.quarantines == 3);
}

static void
test_duplicate_surface_and_capacity(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_identity identity;

	cg_surface_registry_init(&registry);
	for (size_t index = 0; index < CG_SURFACE_REGISTRY_CAPACITY; index++) {
		struct cg_surface_registration_request request =
			request_for((cg_surface_id) index + 1, (uint64_t) index + 1, CG_SURFACE_KIND_APP_VIEW);
		assert(cg_surface_registry_register(&registry, &request, 0) == CG_SURFACE_REGISTRY_OK);
	}
	struct cg_surface_registration_request overflow = request_for(1000, 1000, CG_SURFACE_KIND_APP_VIEW);
	assert(cg_surface_registry_register(&registry, &overflow, 0) == CG_SURFACE_REGISTRY_CAPACITY_EXCEEDED);
	assert(cg_surface_registry_associate(&registry, &registry.entries[0].token, 0x5000, 1, &identity) ==
	       CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_associate(&registry, &registry.entries[1].token, 0x5000, 1, &identity) ==
	       CG_SURFACE_REGISTRY_DUPLICATE_SURFACE);
}

static void
test_scene_retirement(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_registration_request first = request_for(100, 1, CG_SURFACE_KIND_APP_VIEW);
	struct cg_surface_registration_request second = request_for(101, 2, CG_SURFACE_KIND_FIREFOX_VIEW);

	cg_surface_registry_init(&registry);
	assert(cg_surface_registry_register(&registry, &first, 0) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_register(&registry, &second, 0) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_registry_retire_scene(&registry, 7) == 2);
	assert(cg_surface_registry_find(&registry, 7, 100)->state == CG_SURFACE_REGISTRATION_RETIRED);
	assert(cg_surface_registry_find(&registry, 7, 101)->state == CG_SURFACE_REGISTRATION_RETIRED);
	assert(cg_surface_registry_retire_scene(&registry, 7) == 0);
	assert(cg_surface_registry_retire_scene(&registry, 0) == 0);
}

static void
test_reset_zeroes_tokens_and_state(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_registration_request request = request_for(100, 1, CG_SURFACE_KIND_APP_VIEW);
	const uint8_t zero_entries[sizeof(registry.entries)] = {0};
	uint64_t generation;

	cg_surface_registry_init(&registry);
	generation = registry.generation;
	assert(cg_surface_registry_register(&registry, &request, 0) == CG_SURFACE_REGISTRY_OK);
	cg_surface_registry_reset(&registry);
	assert(memcmp(&registry.entries, zero_entries, sizeof(registry.entries)) == 0);
	assert(registry.generation == generation + 1);
	assert(registry.registrations == 0);
	assert(registry.associations == 0);
	assert(registry.quarantines == 0);
}

static void
test_token_hint_round_trip(void)
{
	struct cg_surface_token token = token_for(0x123456789abcdef0ULL);
	struct cg_surface_token decoded;
	char hint[CG_SURFACE_TOKEN_HINT_SIZE];
	char invalid[CG_SURFACE_TOKEN_HINT_SIZE];

	assert(cg_surface_token_hint_encode(&token, hint, sizeof(hint)));
	assert(cg_surface_token_hint_decode(hint, &decoded));
	assert(memcmp(&token, &decoded, sizeof(token)) == 0);
	assert(!cg_surface_token_hint_encode(&token, hint, sizeof(hint) - 1));
	assert(!cg_surface_token_hint_decode(NULL, &decoded));
	assert(!cg_surface_token_hint_decode("linguum-surface-v1:00", &decoded));
	memcpy(invalid, hint, sizeof(invalid));
	invalid[0] = 'x';
	assert(!cg_surface_token_hint_decode(invalid, &decoded));
	memcpy(invalid, hint, sizeof(invalid));
	invalid[CG_SURFACE_TOKEN_HINT_SIZE - 2] = 'z';
	assert(!cg_surface_token_hint_decode(invalid, &decoded));
	memset(invalid, '0', sizeof(invalid));
	memcpy(invalid, CG_SURFACE_TOKEN_HINT_PREFIX, sizeof(CG_SURFACE_TOKEN_HINT_PREFIX) - 1);
	invalid[CG_SURFACE_TOKEN_HINT_SIZE - 1] = '\0';
	assert(!cg_surface_token_hint_decode(invalid, &decoded));
}

int
main(void)
{
	test_registration_and_immutable_association();
	test_invalid_and_duplicate_registration();
	test_parent_rules();
	test_expiry_unknown_surface_and_retirement();
	test_duplicate_surface_and_capacity();
	test_scene_retirement();
	test_reset_zeroes_tokens_and_state();
	test_token_hint_round_trip();
	return 0;
}
