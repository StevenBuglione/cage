#ifndef CG_SURFACE_VIEW_POLICY_H
#define CG_SURFACE_VIEW_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "surface_registry.h"

enum cg_surface_view_state {
	CG_SURFACE_VIEW_UNMANAGED,
	CG_SURFACE_VIEW_ASSOCIATED,
	CG_SURFACE_VIEW_QUARANTINED,
};

struct cg_surface_view_policy {
	enum cg_surface_view_state state;
	enum cg_surface_registry_result association_result;
	struct cg_surface_identity identity;
	uint64_t registry_generation;
};

void cg_surface_view_policy_init(struct cg_surface_view_policy *policy);
bool cg_surface_view_policy_associate(struct cg_surface_view_policy *policy, bool registry_required,
				      struct cg_surface_registry *registry, const char *app_id_hint, uintptr_t surface,
				      uint64_t now_ms);
void cg_surface_view_policy_quarantine(struct cg_surface_view_policy *policy);
bool cg_surface_view_policy_visible(const struct cg_surface_view_policy *policy);
bool cg_surface_view_policy_accepts_input(const struct cg_surface_view_policy *policy);

#endif
