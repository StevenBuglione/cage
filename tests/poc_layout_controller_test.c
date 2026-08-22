#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include "poc_layout_controller.h"

struct apply_state {
	int width;
	uint64_t calls;
};

static bool
apply_width(void *data, int width)
{
	struct apply_state *state = data;

	state->calls++;
	state->width = width;
	return width != 777;
}

static void
send_message(const char *path, const void *message, size_t size)
{
	struct sockaddr_un address = {.sun_family = AF_UNIX};
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);

	assert(fd >= 0);
	strcpy(address.sun_path, path);
	assert(sendto(fd, message, size, 0, (struct sockaddr *) &address, sizeof(address)) == (ssize_t) size);
	assert(close(fd) == 0);
}

static void
dispatch(struct wl_event_loop *event_loop)
{
	assert(wl_event_loop_dispatch(event_loop, 0) == 0);
}

int
main(void)
{
	char runtime_template[] = "/tmp/cage-poc-controller-XXXXXX";
	char path[108];
	struct apply_state apply_state = {0};
	struct cg_poc_layout_controller controller;
	struct wl_display *display = wl_display_create();
	const char *runtime_dir = mkdtemp(runtime_template);

	assert(display);
	assert(runtime_dir);
	assert(snprintf(path, sizeof(path), "%s/layout.sock", runtime_dir) > 0);
	cg_poc_layout_controller_init(&controller);
	assert(cg_poc_layout_controller_start(
		&controller, wl_display_get_event_loop(display), path, runtime_dir, apply_width, &apply_state));
	assert(controller.accepting);
	assert(controller.source);
	assert(!cg_poc_layout_controller_start(
		&controller, wl_display_get_event_loop(display), path, runtime_dir, apply_width, &apply_state));

	send_message(path, "620", 3);
	dispatch(wl_display_get_event_loop(display));
	assert(apply_state.calls == 1);
	assert(apply_state.width == 620);
	assert(controller.accepted_messages == 1);
	assert(controller.rejected_messages == 0);

	send_message(path, "invalid", 7);
	dispatch(wl_display_get_event_loop(display));
	assert(apply_state.calls == 1);
	assert(controller.accepted_messages == 1);
	assert(controller.rejected_messages == 1);

	send_message(path, "777", 3);
	dispatch(wl_display_get_event_loop(display));
	assert(apply_state.calls == 2);
	assert(apply_state.width == 777);
	assert(controller.accepted_messages == 1);
	assert(controller.rejected_messages == 2);

	cg_poc_layout_controller_stop(&controller);
	assert(!controller.accepting);
	assert(!controller.source);
	assert(controller.socket.fd == -1);
	assert(access(path, F_OK) < 0);
	cg_poc_layout_controller_stop(&controller);
	wl_display_destroy(display);
	assert(rmdir(runtime_dir) == 0);
	return 0;
}
