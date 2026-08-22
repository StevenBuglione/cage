#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include "surface_controller.h"

static uint64_t
test_now(void *data)
{
	return *(uint64_t *) data;
}

static int
connect_client(const char *path)
{
	struct sockaddr_un address = {.sun_family = AF_UNIX};
	int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

	assert(fd >= 0);
	strcpy(address.sun_path, path);
	assert(connect(fd, (struct sockaddr *) &address, sizeof(address)) == 0);
	return fd;
}

static void
dispatch(struct wl_event_loop *event_loop)
{
	assert(wl_event_loop_dispatch(event_loop, 0) == 0);
}

static struct cg_surface_registration_request
registration(void)
{
	struct cg_surface_registration_request request = {
		.scene_id = 7,
		.surface_id = 100,
		.kind = CG_SURFACE_KIND_APP_VIEW,
		.association_timeout_ms = 5000,
	};
	request.token.bytes[0] = 1;
	return request;
}

static void
send_bytes(int fd, const uint8_t *bytes, size_t size)
{
	assert(send(fd, bytes, size, 0) == (ssize_t) size);
}

int
main(void)
{
	char runtime_template[] = "/tmp/cage-surface-controller-XXXXXX";
	char path[108];
	struct cg_surface_registry registry;
	struct cg_surface_controller controller;
	struct cg_surface_registration_request register_request = registration();
	struct cg_surface_control_unregister unregister_request = {.scene_id = 7, .surface_id = 100};
	struct wl_display *display = wl_display_create();
	struct wl_event_loop *event_loop;
	struct stat metadata;
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE + 1];
	uint64_t now = 1000;
	size_t size;
	int client;
	const char *runtime_dir = mkdtemp(runtime_template);

	assert(display);
	assert(runtime_dir);
	event_loop = wl_display_get_event_loop(display);
	assert(snprintf(path, sizeof(path), "%s/control.sock", runtime_dir) > 0);
	cg_surface_registry_init(&registry);
	cg_surface_controller_init(&controller);
	assert(cg_surface_controller_start(&controller, event_loop, path, runtime_dir, &registry, test_now, &now));
	assert(!cg_surface_controller_start(&controller, event_loop, path, runtime_dir, &registry, test_now, &now));
	assert(stat(path, &metadata) == 0);
	assert((metadata.st_mode & 0777) == 0600);

	client = connect_client(path);
	dispatch(event_loop);
	assert(controller.client_fd >= 0);
	assert(controller.client_source);

	assert(cg_surface_control_encode_register(&register_request, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(controller.applied_messages == 1);
	assert(controller.rejected_messages == 0);
	assert(cg_surface_registry_find(&registry, 7, 100)->state == CG_SURFACE_REGISTRATION_PENDING);

	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(controller.rejected_messages == 1);
	assert(controller.last_registry_result == CG_SURFACE_REGISTRY_DUPLICATE_ID);

	bytes[0] = 'X';
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(controller.rejected_messages == 2);
	assert(controller.last_parse_result == CG_SURFACE_CONTROL_PARSE_INVALID_MAGIC);

	memset(bytes, 0, sizeof(bytes));
	send_bytes(client, bytes, sizeof(bytes));
	dispatch(event_loop);
	assert(controller.rejected_messages == 3);
	assert(controller.last_parse_result == CG_SURFACE_CONTROL_PARSE_INVALID_SIZE);

	assert(cg_surface_control_encode_unregister(&unregister_request, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(controller.applied_messages == 2);
	assert(cg_surface_registry_find(&registry, 7, 100)->state == CG_SURFACE_REGISTRATION_RETIRED);

	assert(cg_surface_control_encode_reset(bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(controller.applied_messages == 3);
	assert(registry.registrations == 0);

	register_request.surface_id = 101;
	register_request.token.bytes[0] = 2;
	assert(cg_surface_control_encode_register(&register_request, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(registry.registrations == 1);
	assert(close(client) == 0);
	dispatch(event_loop);
	assert(controller.client_fd == -1);
	assert(!controller.client_source);
	assert(controller.disconnects == 1);
	assert(registry.registrations == 0);

	client = connect_client(path);
	dispatch(event_loop);
	assert(controller.client_fd >= 0);
	assert(close(client) == 0);
	dispatch(event_loop);
	assert(controller.disconnects == 2);

	cg_surface_controller_stop(&controller);
	assert(!controller.accepting);
	assert(controller.listener_fd == -1);
	assert(!controller.listener_source);
	assert(access(path, F_OK) < 0);
	cg_surface_controller_stop(&controller);
	wl_display_destroy(display);
	assert(rmdir(runtime_dir) == 0);
	return 0;
}
