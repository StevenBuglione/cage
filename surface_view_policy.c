/*
 * Cage: A Wayland kiosk.
 *
 * Fail-closed view association and quarantine policy.
 *
 * See the LICENSE file accompanying this file.
 */

#include <string.h>

#include "surface_token_hint.h"
#include "surface_view_policy.h"

static bool
has_token_namespace(const char *app_id_hint)
{
	const size_t prefix_length = sizeof(CG_SURFACE_TOKEN_HINT_PREFIX) - 1;

	return app_id_hint && strncmp(app_id_hint, CG_SURFACE_TOKEN_HINT_PREFIX, prefix_length) == 0;
}

static bool
associate_token(struct cg_surface_view_policy *policy, struct cg_surface_registry *registry,
		const struct cg_surface_token *token, uintptr_t surface, uint64_t now_ms)
{
	policy->association_result =
		cg_surface_registry_associate(registry, token, surface, now_ms, &policy->identity);
	if (policy->association_result != CG_SURFACE_REGISTRY_OK) {
		cg_surface_view_policy_quarantine(policy);
		return false;
	}
	policy->state = CG_SURFACE_VIEW_ASSOCIATED;
	policy->registry_generation = registry->generation;
	return true;
}

void
cg_surface_view_policy_init(struct cg_surface_view_policy *policy)
{
	if (!policy) {
		return;
	}
	memset(policy, 0, sizeof(*policy));
	policy->state = CG_SURFACE_VIEW_UNMANAGED;
	policy->association_result = CG_SURFACE_REGISTRY_NOT_FOUND;
}

void
cg_surface_view_policy_quarantine(struct cg_surface_view_policy *policy)
{
	if (!policy) {
		return;
	}
	policy->state = CG_SURFACE_VIEW_QUARANTINED;
	memset(&policy->identity, 0, sizeof(policy->identity));
	policy->registry_generation = 0;
}

bool
cg_surface_view_policy_associate(struct cg_surface_view_policy *policy, bool registry_required,
				 struct cg_surface_registry *registry, const char *app_id_hint, uintptr_t surface,
				 uint64_t now_ms)
{
	struct cg_surface_token token = {0};

	if (!policy) {
		return false;
	}
	if (policy->state == CG_SURFACE_VIEW_ASSOCIATED) {
		if (registry_required && (!registry || policy->registry_generation != registry->generation)) {
			policy->association_result = CG_SURFACE_REGISTRY_NOT_FOUND;
			cg_surface_view_policy_quarantine(policy);
			return false;
		}
		return true;
	}
	if (policy->state == CG_SURFACE_VIEW_QUARANTINED) {
		return false;
	}
	if (!registry_required) {
		return true;
	}
	if (!registry) {
		policy->association_result = CG_SURFACE_REGISTRY_INVALID;
		cg_surface_view_policy_quarantine(policy);
		return false;
	}

	if (!cg_surface_token_hint_decode(app_id_hint, &token)) {
		if (!has_token_namespace(app_id_hint)) {
			policy->state = CG_SURFACE_VIEW_PENDING;
			policy->association_result = CG_SURFACE_REGISTRY_NOT_FOUND;
			return false;
		}
		policy->association_result = CG_SURFACE_REGISTRY_INVALID;
		cg_surface_view_policy_quarantine(policy);
		return false;
	}
	return associate_token(policy, registry, &token, surface, now_ms);
}

bool
cg_surface_view_policy_associate_pending(struct cg_surface_view_policy *policy,
					 struct cg_surface_registry *registry, const char *app_id_hint,
					 uintptr_t surface, uint64_t now_ms)
{
	struct cg_surface_token token = {0};

	if (!policy || policy->state != CG_SURFACE_VIEW_PENDING) {
		return false;
	}
	if (!has_token_namespace(app_id_hint)) {
		return false;
	}
	if (!registry || !cg_surface_token_hint_decode(app_id_hint, &token)) {
		policy->association_result = CG_SURFACE_REGISTRY_INVALID;
		cg_surface_view_policy_quarantine(policy);
		return false;
	}
	return associate_token(policy, registry, &token, surface, now_ms);
}

bool
cg_surface_view_policy_visible(const struct cg_surface_view_policy *policy)
{
	return policy && (policy->state == CG_SURFACE_VIEW_UNMANAGED || policy->state == CG_SURFACE_VIEW_ASSOCIATED);
}

bool
cg_surface_view_policy_accepts_input(const struct cg_surface_view_policy *policy)
{
	return cg_surface_view_policy_visible(policy);
}
