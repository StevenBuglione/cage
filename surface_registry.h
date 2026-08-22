#ifndef CG_SURFACE_REGISTRY_H
#define CG_SURFACE_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CG_SURFACE_TOKEN_SIZE 32
#define CG_SURFACE_REGISTRY_CAPACITY 128
#define CG_SURFACE_ASSOCIATION_TIMEOUT_MAX_MS 60000

typedef uint64_t cg_scene_id;
typedef uint64_t cg_surface_id;

struct cg_surface_token {
	uint8_t bytes[CG_SURFACE_TOKEN_SIZE];
};

enum cg_surface_kind {
	CG_SURFACE_KIND_APP_VIEW = 1,
	CG_SURFACE_KIND_FIREFOX_VIEW = 2,
	CG_SURFACE_KIND_OVERLAY = 3,
	CG_SURFACE_KIND_POPUP = 4,
};

enum cg_surface_registration_state {
	CG_SURFACE_REGISTRATION_PENDING,
	CG_SURFACE_REGISTRATION_ASSOCIATED,
	CG_SURFACE_REGISTRATION_EXPIRED,
	CG_SURFACE_REGISTRATION_RETIRED,
};

enum cg_surface_registry_result {
	CG_SURFACE_REGISTRY_OK,
	CG_SURFACE_REGISTRY_INVALID,
	CG_SURFACE_REGISTRY_CAPACITY_EXCEEDED,
	CG_SURFACE_REGISTRY_DUPLICATE_ID,
	CG_SURFACE_REGISTRY_DUPLICATE_TOKEN,
	CG_SURFACE_REGISTRY_PARENT_NOT_FOUND,
	CG_SURFACE_REGISTRY_UNKNOWN_TOKEN,
	CG_SURFACE_REGISTRY_TOKEN_EXPIRED,
	CG_SURFACE_REGISTRY_TOKEN_REPLAYED,
	CG_SURFACE_REGISTRY_DUPLICATE_SURFACE,
	CG_SURFACE_REGISTRY_NOT_FOUND,
};

struct cg_surface_registration_request {
	cg_scene_id scene_id;
	cg_surface_id surface_id;
	struct cg_surface_token token;
	enum cg_surface_kind kind;
	bool has_parent;
	cg_surface_id parent_surface_id;
	uint32_t association_timeout_ms;
};

struct cg_surface_identity {
	cg_scene_id scene_id;
	cg_surface_id surface_id;
	enum cg_surface_kind kind;
	bool has_parent;
	cg_surface_id parent_surface_id;
};

struct cg_surface_registration {
	bool occupied;
	struct cg_surface_identity identity;
	struct cg_surface_token token;
	enum cg_surface_registration_state state;
	uint64_t association_deadline_ms;
	uintptr_t associated_surface;
};

struct cg_surface_registry {
	struct cg_surface_registration entries[CG_SURFACE_REGISTRY_CAPACITY];
	size_t registrations;
	uint64_t associations;
	uint64_t quarantines;
	uint64_t expirations;
	uint64_t rejected_requests;
};

void cg_surface_registry_init(struct cg_surface_registry *registry);
void cg_surface_registry_reset(struct cg_surface_registry *registry);
bool cg_surface_token_is_valid(const struct cg_surface_token *token);
bool cg_surface_kind_is_valid(enum cg_surface_kind kind);
enum cg_surface_registry_result cg_surface_registry_register(struct cg_surface_registry *registry,
	const struct cg_surface_registration_request *request, uint64_t now_ms);
enum cg_surface_registry_result cg_surface_registry_associate(struct cg_surface_registry *registry,
	const struct cg_surface_token *token, uintptr_t surface, uint64_t now_ms, struct cg_surface_identity *identity_out);
enum cg_surface_registry_result cg_surface_registry_retire(
	struct cg_surface_registry *registry, cg_scene_id scene_id, cg_surface_id surface_id);
size_t cg_surface_registry_expire(struct cg_surface_registry *registry, uint64_t now_ms);
const struct cg_surface_registration *cg_surface_registry_find(
	const struct cg_surface_registry *registry, cg_scene_id scene_id, cg_surface_id surface_id);

#endif
