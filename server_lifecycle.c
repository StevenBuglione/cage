/*
 * Cage: A Wayland kiosk.
 *
 * See the LICENSE file accompanying this file.
 */

#include <wayland-server-core.h>

#include "server.h"

void
server_terminate(struct cg_server *server)
{
	/* Work around https://gitlab.freedesktop.org/wayland/wayland/-/merge_requests/421
	 * and expose teardown state before backend output destruction begins. */
	if (server->terminated) {
		return;
	}

	server->terminated = true;
	wl_display_terminate(server->wl_display);
}
