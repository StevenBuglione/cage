#ifndef CG_SURFACE_TOKEN_HINT_H
#define CG_SURFACE_TOKEN_HINT_H

#include <stdbool.h>
#include <stddef.h>

#include "surface_registry.h"

#define CG_SURFACE_TOKEN_HINT_PREFIX "linguum-surface-v1:"
#define CG_SURFACE_TOKEN_HINT_SIZE (sizeof(CG_SURFACE_TOKEN_HINT_PREFIX) - 1 + CG_SURFACE_TOKEN_SIZE * 2 + 1)

bool cg_surface_token_hint_encode(
	const struct cg_surface_token *token, char *hint_out, size_t hint_capacity);
bool cg_surface_token_hint_decode(const char *hint, struct cg_surface_token *token_out);

#endif
