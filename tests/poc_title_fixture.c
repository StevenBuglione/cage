/* Frozen characterization fixture for the pre-framework POC. */

#include <string.h>

#include "poc_title_fixture.h"

#define POC_WORKSPACE_TITLE "Linguum Workspace"
#define POC_CONTROLS_TITLE "Linguum Browser Controls"

enum cg_poc_surface_role
cg_poc_title_fixture_classify(bool enabled, const char *title)
{
	if (!enabled) {
		return CG_POC_SURFACE_DEFAULT;
	}
	if (title && strncmp(title, POC_WORKSPACE_TITLE, strlen(POC_WORKSPACE_TITLE)) == 0) {
		return CG_POC_SURFACE_WORKSPACE;
	}
	if (title && strncmp(title, POC_CONTROLS_TITLE, strlen(POC_CONTROLS_TITLE)) == 0) {
		return CG_POC_SURFACE_CONTROLS;
	}
	return CG_POC_SURFACE_BROWSER;
}
