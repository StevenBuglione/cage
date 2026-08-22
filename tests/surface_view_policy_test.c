#include <assert.h>
#include <string.h>

#include "surface_token_hint.h"
#include "surface_view_policy.h"

static struct cg_surface_registration_request
registration(uint64_t token_value)
{
	struct cg_surface_registration_request request = {
		.scene_id = 7,
		.surface_id = 100,
		.kind = CG_SURFACE_KIND_FIREFOX_VIEW,
		.association_timeout_ms = 5000,
	};
	request.token.bytes[0] = (uint8_t) token_value;
	request.token.bytes[CG_SURFACE_TOKEN_SIZE - 1] = 0xa5;
	return request;
}

static void
test_unmanaged_mode(void)
{
	struct cg_surface_view_policy policy;

	cg_surface_view_policy_init(&policy);
	assert(cg_surface_view_policy_associate(&policy, false, NULL, NULL, 1, 0));
	assert(policy.state == CG_SURFACE_VIEW_UNMANAGED);
	assert(cg_surface_view_policy_visible(&policy));
	assert(cg_surface_view_policy_accepts_input(&policy));
}

static void
test_association_is_immutable(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_view_policy policy;
	struct cg_surface_registration_request request = registration(1);
	char hint[CG_SURFACE_TOKEN_HINT_SIZE];

	cg_surface_registry_init(&registry);
	cg_surface_view_policy_init(&policy);
	assert(cg_surface_registry_register(&registry, &request, 100) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_token_hint_encode(&request.token, hint, sizeof(hint)));
	assert(cg_surface_view_policy_associate(&policy, true, &registry, hint, 0x1000, 200));
	assert(policy.state == CG_SURFACE_VIEW_ASSOCIATED);
	assert(policy.identity.scene_id == 7);
	assert(policy.identity.surface_id == 100);
	assert(policy.identity.kind == CG_SURFACE_KIND_FIREFOX_VIEW);
	assert(cg_surface_view_policy_visible(&policy));
	assert(cg_surface_view_policy_accepts_input(&policy));

	assert(cg_surface_view_policy_associate(&policy, true, &registry, "changed-app-id", 0x2000, 300));
	assert(policy.identity.surface_id == 100);
	assert(registry.associations == 1);
	assert(registry.quarantines == 0);
}

static void
test_unknown_invalid_stale_and_replayed_quarantine(void)
{
	struct cg_surface_registry registry;
	struct cg_surface_view_policy invalid;
	struct cg_surface_view_policy stale;
	struct cg_surface_view_policy associated;
	struct cg_surface_view_policy replay;
	struct cg_surface_registration_request request = registration(1);
	char hint[CG_SURFACE_TOKEN_HINT_SIZE];

	cg_surface_registry_init(&registry);
	cg_surface_view_policy_init(&invalid);
	assert(!cg_surface_view_policy_associate(&invalid, true, &registry, "invalid", 0x1000, 0));
	assert(invalid.state == CG_SURFACE_VIEW_QUARANTINED);
	assert(!cg_surface_view_policy_visible(&invalid));
	assert(!cg_surface_view_policy_accepts_input(&invalid));
	assert(!cg_surface_view_policy_associate(&invalid, true, &registry, NULL, 0x1000, 0));

	request.association_timeout_ms = 10;
	assert(cg_surface_registry_register(&registry, &request, 100) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_token_hint_encode(&request.token, hint, sizeof(hint)));
	cg_surface_view_policy_init(&stale);
	assert(!cg_surface_view_policy_associate(&stale, true, &registry, hint, 0x2000, 110));
	assert(stale.association_result == CG_SURFACE_REGISTRY_TOKEN_EXPIRED);

	cg_surface_registry_reset(&registry);
	request = registration(2);
	assert(cg_surface_registry_register(&registry, &request, 0) == CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_token_hint_encode(&request.token, hint, sizeof(hint)));
	cg_surface_view_policy_init(&associated);
	assert(cg_surface_view_policy_associate(&associated, true, &registry, hint, 0x3000, 1));
	cg_surface_view_policy_init(&replay);
	assert(!cg_surface_view_policy_associate(&replay, true, &registry, hint, 0x4000, 2));
	assert(replay.association_result == CG_SURFACE_REGISTRY_TOKEN_REPLAYED);
}

static void
test_forced_quarantine_erases_identity(void)
{
	struct cg_surface_view_policy policy = {
		.state = CG_SURFACE_VIEW_ASSOCIATED,
		.identity = {.scene_id = 7, .surface_id = 100, .kind = CG_SURFACE_KIND_APP_VIEW},
	};
	struct cg_surface_identity zero = {0};

	cg_surface_view_policy_quarantine(&policy);
	assert(policy.state == CG_SURFACE_VIEW_QUARANTINED);
	assert(memcmp(&policy.identity, &zero, sizeof(zero)) == 0);
	assert(!cg_surface_view_policy_visible(&policy));
	assert(!cg_surface_view_policy_accepts_input(&policy));
}

int
main(void)
{
	test_unmanaged_mode();
	test_association_is_immutable();
	test_unknown_invalid_stale_and_replayed_quarantine();
	test_forced_quarantine_erases_identity();
	return 0;
}
