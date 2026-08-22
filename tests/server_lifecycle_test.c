#include <assert.h>
#include <wayland-server-core.h>

#include "server.h"

int
main(void)
{
	struct cg_server server = {0};
	server.wl_display = wl_display_create();
	assert(server.wl_display != NULL);
	assert(!server.terminated);

	server_terminate(&server);
	assert(server.terminated);

	/* Repeated termination is intentionally idempotent. */
	server_terminate(&server);
	assert(server.terminated);

	wl_display_destroy(server.wl_display);
	return 0;
}
