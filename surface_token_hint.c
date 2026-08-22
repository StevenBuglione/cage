/*
 * Cage: A Wayland kiosk.
 *
 * Versioned framework-controlled app_id hint for one-time surface association.
 *
 * See the LICENSE file accompanying this file.
 */

#include <string.h>

#include "surface_token_hint.h"

static int
hex_value(char character)
{
	if (character >= '0' && character <= '9') {
		return character - '0';
	}
	if (character >= 'a' && character <= 'f') {
		return character - 'a' + 10;
	}
	if (character >= 'A' && character <= 'F') {
		return character - 'A' + 10;
	}
	return -1;
}

bool
cg_surface_token_hint_encode(const struct cg_surface_token *token, char *hint_out, size_t hint_capacity)
{
	static const char hex[] = "0123456789abcdef";
	const size_t prefix_length = sizeof(CG_SURFACE_TOKEN_HINT_PREFIX) - 1;

	if (!cg_surface_token_is_valid(token) || !hint_out || hint_capacity < CG_SURFACE_TOKEN_HINT_SIZE) {
		return false;
	}
	memcpy(hint_out, CG_SURFACE_TOKEN_HINT_PREFIX, prefix_length);
	for (size_t index = 0; index < CG_SURFACE_TOKEN_SIZE; index++) {
		hint_out[prefix_length + index * 2] = hex[token->bytes[index] >> 4];
		hint_out[prefix_length + index * 2 + 1] = hex[token->bytes[index] & 0x0f];
	}
	hint_out[CG_SURFACE_TOKEN_HINT_SIZE - 1] = '\0';
	return true;
}

bool
cg_surface_token_hint_decode(const char *hint, struct cg_surface_token *token_out)
{
	const size_t prefix_length = sizeof(CG_SURFACE_TOKEN_HINT_PREFIX) - 1;
	struct cg_surface_token decoded = {0};

	if (!hint || !token_out || strlen(hint) != CG_SURFACE_TOKEN_HINT_SIZE - 1 ||
	    strncmp(hint, CG_SURFACE_TOKEN_HINT_PREFIX, prefix_length) != 0) {
		return false;
	}
	for (size_t index = 0; index < CG_SURFACE_TOKEN_SIZE; index++) {
		int high = hex_value(hint[prefix_length + index * 2]);
		int low = hex_value(hint[prefix_length + index * 2 + 1]);
		if (high < 0 || low < 0) {
			return false;
		}
		decoded.bytes[index] = (uint8_t) ((high << 4) | low);
	}
	if (!cg_surface_token_is_valid(&decoded)) {
		return false;
	}
	*token_out = decoded;
	return true;
}
