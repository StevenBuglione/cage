#ifndef CG_POC_LAYOUT_H
#define CG_POC_LAYOUT_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Temporary characterization boundary for the original Linguum POC patch.
 * The framework scene model replaces this title-based API in M1-WP03/04.
 */

enum cg_poc_surface_role {
	CG_POC_SURFACE_DEFAULT,
	CG_POC_SURFACE_WORKSPACE,
	CG_POC_SURFACE_CONTROLS,
	CG_POC_SURFACE_BROWSER,
};

struct cg_poc_rect {
	int x;
	int y;
	int width;
	int height;
};

bool cg_poc_layout_parse_width(const char *value, int *width_out);
bool cg_poc_layout_parse_message(const char *message, size_t size, int *width_out);
bool cg_poc_layout_socket_path_valid(const char *path, const char *runtime_dir, size_t path_capacity);
enum cg_poc_surface_role cg_poc_layout_classify_title(bool enabled, const char *title);
bool cg_poc_layout_rect(struct cg_poc_rect output, int browser_width, enum cg_poc_surface_role role,
			struct cg_poc_rect *target_out);

#endif
