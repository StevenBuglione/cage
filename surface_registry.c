/*
 * Cage: A Wayland kiosk.
 *
 * Generic surface registration and one-time association registry.
 *
 * See the LICENSE file accompanying this file.
 */

#include <limits.h>
#include <string.h>

#include "surface_registry.h"

static void
secure_zero(void *memory, size_t size)
{
	volatile uint8_t *cursor = memory;

	while (size > 0) {
		*cursor++ = 0;
		size--;
	}
}

static bool
token_equal(const struct cg_surface_token *left, const struct cg_surface_token *right)
{
	uint8_t difference = 0;

	for (size_t index = 0; index < CG_SURFACE_TOKEN_SIZE; index++) {
		difference |= left->bytes[index] ^ right->bytes[index];
	}
	return difference == 0;
}

bool
cg_surface_token_is_valid(const struct cg_surface_token *token)
{
	uint8_t aggregate = 0;

	if (!token) {
		return false;
	}
	for (size_t index = 0; index < CG_SURFACE_TOKEN_SIZE; index++) {
		aggregate |= token->bytes[index];
	}
	return aggregate != 0;
}

bool
cg_surface_kind_is_valid(enum cg_surface_kind kind)
{
	return kind >= CG_SURFACE_KIND_APP_VIEW && kind <= CG_SURFACE_KIND_POPUP;
}

void
cg_surface_registry_init(struct cg_surface_registry *registry)
{
	if (!registry) {
		return;
	}
	memset(registry, 0, sizeof(*registry));
}

void
cg_surface_registry_reset(struct cg_surface_registry *registry)
{
	if (!registry) {
		return;
	}
	secure_zero(registry, sizeof(*registry));
}

const struct cg_surface_registration *
cg_surface_registry_find(const struct cg_surface_registry *registry, cg_scene_id scene_id, cg_surface_id surface_id)
{
	if (!registry || scene_id == 0 || surface_id == 0) {
		return NULL;
	}
	for (size_t index = 0; index < CG_SURFACE_REGISTRY_CAPACITY; index++) {
		const struct cg_surface_registration *entry = &registry->entries[index];
		if (entry->occupied && entry->identity.scene_id == scene_id && entry->identity.surface_id == surface_id) {
			return entry;
		}
	}
	return NULL;
}

static bool
parent_exists(const struct cg_surface_registry *registry, const struct cg_surface_registration_request *request)
{
	const struct cg_surface_registration *parent;

	if (!request->has_parent) {
		return request->kind != CG_SURFACE_KIND_POPUP && request->parent_surface_id == 0;
	}
	if (request->parent_surface_id == 0 || request->parent_surface_id == request->surface_id) {
		return false;
	}
	parent = cg_surface_registry_find(registry, request->scene_id, request->parent_surface_id);
	return parent && parent->state != CG_SURFACE_REGISTRATION_EXPIRED &&
	       parent->state != CG_SURFACE_REGISTRATION_RETIRED;
}

enum cg_surface_registry_result
cg_surface_registry_register(struct cg_surface_registry *registry, const struct cg_surface_registration_request *request,
	uint64_t now_ms)
{
	struct cg_surface_registration *available = NULL;

	if (!registry || !request || request->scene_id == 0 || request->surface_id == 0 ||
	    !cg_surface_token_is_valid(&request->token) || !cg_surface_kind_is_valid(request->kind) ||
	    request->association_timeout_ms == 0 ||
	    request->association_timeout_ms > CG_SURFACE_ASSOCIATION_TIMEOUT_MAX_MS ||
	    now_ms > UINT64_MAX - request->association_timeout_ms) {
		if (registry) {
			registry->rejected_requests++;
		}
		return CG_SURFACE_REGISTRY_INVALID;
	}

	for (size_t index = 0; index < CG_SURFACE_REGISTRY_CAPACITY; index++) {
		struct cg_surface_registration *entry = &registry->entries[index];
		if (!entry->occupied) {
			if (!available) {
				available = entry;
			}
			continue;
		}
		if (entry->identity.scene_id == request->scene_id && entry->identity.surface_id == request->surface_id) {
			registry->rejected_requests++;
			return CG_SURFACE_REGISTRY_DUPLICATE_ID;
		}
		if (token_equal(&entry->token, &request->token)) {
			registry->rejected_requests++;
			return CG_SURFACE_REGISTRY_DUPLICATE_TOKEN;
		}
	}

	if (!parent_exists(registry, request)) {
		registry->rejected_requests++;
		return CG_SURFACE_REGISTRY_PARENT_NOT_FOUND;
	}
	if (!available) {
		registry->rejected_requests++;
		return CG_SURFACE_REGISTRY_CAPACITY_EXCEEDED;
	}

	available->occupied = true;
	available->identity.scene_id = request->scene_id;
	available->identity.surface_id = request->surface_id;
	available->identity.kind = request->kind;
	available->identity.has_parent = request->has_parent;
	available->identity.parent_surface_id = request->parent_surface_id;
	available->token = request->token;
	available->state = CG_SURFACE_REGISTRATION_PENDING;
	available->association_deadline_ms = now_ms + request->association_timeout_ms;
	available->associated_surface = 0;
	registry->registrations++;
	return CG_SURFACE_REGISTRY_OK;
}

size_t
cg_surface_registry_expire(struct cg_surface_registry *registry, uint64_t now_ms)
{
	size_t expired = 0;

	if (!registry) {
		return 0;
	}
	for (size_t index = 0; index < CG_SURFACE_REGISTRY_CAPACITY; index++) {
		struct cg_surface_registration *entry = &registry->entries[index];
		if (entry->occupied && entry->state == CG_SURFACE_REGISTRATION_PENDING &&
		    now_ms >= entry->association_deadline_ms) {
			entry->state = CG_SURFACE_REGISTRATION_EXPIRED;
			entry->associated_surface = 0;
			expired++;
		}
	}
	registry->expirations += expired;
	return expired;
}

enum cg_surface_registry_result
cg_surface_registry_associate(struct cg_surface_registry *registry, const struct cg_surface_token *token,
	uintptr_t surface, uint64_t now_ms, struct cg_surface_identity *identity_out)
{
	if (!registry || !cg_surface_token_is_valid(token) || surface == 0 || !identity_out) {
		if (registry) {
			registry->quarantines++;
		}
		return CG_SURFACE_REGISTRY_INVALID;
	}

	cg_surface_registry_expire(registry, now_ms);
	for (size_t index = 0; index < CG_SURFACE_REGISTRY_CAPACITY; index++) {
		struct cg_surface_registration *entry = &registry->entries[index];
		if (!entry->occupied || !token_equal(&entry->token, token)) {
			continue;
		}
		if (entry->state == CG_SURFACE_REGISTRATION_EXPIRED) {
			registry->quarantines++;
			return CG_SURFACE_REGISTRY_TOKEN_EXPIRED;
		}
		if (entry->state == CG_SURFACE_REGISTRATION_ASSOCIATED || entry->state == CG_SURFACE_REGISTRATION_RETIRED) {
			registry->quarantines++;
			return CG_SURFACE_REGISTRY_TOKEN_REPLAYED;
		}
		for (size_t surface_index = 0; surface_index < CG_SURFACE_REGISTRY_CAPACITY; surface_index++) {
			const struct cg_surface_registration *other = &registry->entries[surface_index];
			if (other->occupied && other->state == CG_SURFACE_REGISTRATION_ASSOCIATED &&
			    other->associated_surface == surface) {
				registry->quarantines++;
				return CG_SURFACE_REGISTRY_DUPLICATE_SURFACE;
			}
		}
		entry->state = CG_SURFACE_REGISTRATION_ASSOCIATED;
		entry->associated_surface = surface;
		*identity_out = entry->identity;
		registry->associations++;
		return CG_SURFACE_REGISTRY_OK;
	}

	registry->quarantines++;
	return CG_SURFACE_REGISTRY_UNKNOWN_TOKEN;
}

enum cg_surface_registry_result
cg_surface_registry_retire(struct cg_surface_registry *registry, cg_scene_id scene_id, cg_surface_id surface_id)
{
	if (!registry || scene_id == 0 || surface_id == 0) {
		return CG_SURFACE_REGISTRY_INVALID;
	}
	for (size_t index = 0; index < CG_SURFACE_REGISTRY_CAPACITY; index++) {
		struct cg_surface_registration *entry = &registry->entries[index];
		if (!entry->occupied || entry->identity.scene_id != scene_id || entry->identity.surface_id != surface_id) {
			continue;
		}
		if (entry->state == CG_SURFACE_REGISTRATION_RETIRED) {
			return CG_SURFACE_REGISTRY_NOT_FOUND;
		}
		entry->state = CG_SURFACE_REGISTRATION_RETIRED;
		entry->associated_surface = 0;
		return CG_SURFACE_REGISTRY_OK;
	}
	return CG_SURFACE_REGISTRY_NOT_FOUND;
}
