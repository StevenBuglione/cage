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

struct observed_events {
	uint64_t resets;
	uint64_t retired;
	uint64_t scene_applied;
	uint64_t scene_destroyed;
	uint64_t output_resized;
	cg_scene_id last_scene_id;
	cg_surface_id last_surface_id;
	cg_scene_revision last_revision;
};

static void
observe_event(const struct cg_surface_controller_event *event, void *data)
{
	struct observed_events *observed = data;

	if (event->type == CG_SURFACE_CONTROLLER_RESET) {
		observed->resets++;
	} else if (event->type == CG_SURFACE_CONTROLLER_RETIRED) {
		observed->retired++;
		observed->last_scene_id = event->scene_id;
		observed->last_surface_id = event->surface_id;
	} else if (event->type == CG_SURFACE_CONTROLLER_SCENE_APPLIED) {
		observed->scene_applied++;
		observed->last_scene_id = event->scene_id;
		observed->last_revision = event->revision;
	} else if (event->type == CG_SURFACE_CONTROLLER_SCENE_DESTROYED) {
		observed->scene_destroyed++;
		observed->last_scene_id = event->scene_id;
	} else if (event->type == CG_SURFACE_CONTROLLER_OUTPUT_RESIZED) {
		observed->output_resized++;
		observed->last_scene_id = event->scene_id;
		observed->last_revision = event->revision;
	}
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
	struct cg_scene_model scenes;
	struct cg_surface_controller controller;
	struct cg_surface_registration_request register_request = registration();
	struct cg_surface_identity associated_identity;
	struct cg_surface_control_unregister unregister_request = {.scene_id = 7, .surface_id = 100};
	struct observed_events observed = {0};
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
	cg_scene_model_init(&scenes);
	cg_surface_controller_init(&controller);
	assert(cg_surface_controller_start(&controller, event_loop, path, runtime_dir, &registry, &scenes, test_now, &now,
					   observe_event, &observed));
	assert(!cg_surface_controller_start(&controller, event_loop, path, runtime_dir, &registry, &scenes, test_now, &now,
					    observe_event, &observed));
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
	assert(cg_surface_registry_associate(&registry, &register_request.token, 0x1000, now, &associated_identity) ==
	       CG_SURFACE_REGISTRY_OK);
	assert(cg_surface_controller_notify_associated(&controller, &associated_identity));
	assert(recv(client, bytes, sizeof(bytes), 0) == CG_SURFACE_CONTROL_ASSOCIATED_SIZE);
	assert(memcmp(bytes, "LSC1\x01\x81\x00\x28", 8) == 0);
	assert(cg_surface_controller_now(&controller) == now);

	assert(cg_surface_control_encode_register(&register_request, bytes, sizeof(bytes), &size));
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
	assert(observed.retired == 1);
	assert(observed.last_scene_id == 7);
	assert(observed.last_surface_id == 100);

	assert(cg_surface_control_encode_reset(bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(controller.applied_messages == 3);
	assert(registry.registrations == 0);
	assert(observed.resets == 1);

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
	assert(observed.resets == 2);

	client = connect_client(path);
	dispatch(event_loop);
	assert(controller.client_fd >= 0);
	const struct cg_surface_control_create_scene create_scene = {
		.scene_id = 7,
		.output_id = 11,
		.output_width = 1000,
		.output_height = 700,
	};
	assert(cg_surface_control_encode_create_scene(&create_scene, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(cg_scene_model_find(&scenes, 7));

	register_request.surface_id = 102;
	register_request.token.bytes[0] = 3;
	assert(cg_surface_control_encode_register(&register_request, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	struct cg_scene_snapshot snapshot = {
		.scene_id = 7,
		.output_id = 11,
		.revision = 1,
		.surface_count = 1,
		.has_focused_surface = true,
		.focused_surface_id = 102,
	};
	snapshot.surfaces[0] = (struct cg_scene_surface_state) {
		.surface_id = 102,
		.bounds = {.width = 1000, .height = 700},
		.visible = true,
		.accepts_input = true,
	};
	assert(cg_surface_control_encode_apply_scene(&snapshot, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(observed.scene_applied == 1 && observed.last_revision == 1);
	assert(cg_scene_model_find(&scenes, 7)->snapshot.revision == 1);

	const struct cg_surface_control_resize_output resize_output = {
		.scene_id = 7,
		.output_id = 11,
		.output_width = 1200,
		.output_height = 800,
	};
	assert(cg_surface_control_encode_resize_output(&resize_output, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(observed.output_resized == 1);
	assert(cg_scene_model_find(&scenes, 7)->output_width == 1200);

	const struct cg_surface_control_destroy_scene destroy_scene = {.scene_id = 7};
	assert(cg_surface_control_encode_destroy_scene(&destroy_scene, bytes, sizeof(bytes), &size));
	send_bytes(client, bytes, size);
	dispatch(event_loop);
	assert(observed.scene_destroyed == 1);
	assert(!cg_scene_model_find(&scenes, 7));
	assert(cg_surface_registry_find(&registry, 7, 102)->state == CG_SURFACE_REGISTRATION_RETIRED);
	assert(controller.applied_messages == 9);
	assert(close(client) == 0);
	dispatch(event_loop);
	assert(controller.disconnects == 2);
	assert(observed.resets == 3);

	cg_surface_controller_stop(&controller);
	assert(!controller.accepting);
	assert(controller.listener_fd == -1);
	assert(!controller.listener_source);
	assert(access(path, F_OK) < 0);
	assert(observed.resets == 4);
	cg_surface_controller_stop(&controller);
	wl_display_destroy(display);
	assert(rmdir(runtime_dir) == 0);
	return 0;
}
