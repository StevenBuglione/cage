/*
 * Cage: A Wayland kiosk.
 *
 * Temporary event-loop adapter for the original Linguum POC layout socket.
 * The typed controller transport replaces this file in M1-WP03/M2.
 *
 * See the LICENSE file accompanying this file.
 */

#include <errno.h>
#include <stddef.h>

#include "poc_layout_controller.h"

static int
handle_layout_message(int fd, uint32_t mask, void *data)
{
	struct cg_poc_layout_controller *controller = data;
	int width;

	if (!controller->accepting || !(mask & WL_EVENT_READABLE)) {
		return 0;
	}

	controller->last_result = cg_poc_layout_socket_receive(&controller->socket, &width);
	switch (controller->last_result) {
	case CG_POC_LAYOUT_RECEIVE_NONE:
		break;
	case CG_POC_LAYOUT_RECEIVE_WIDTH:
		if (controller->apply_width(controller->data, width)) {
			controller->accepted_messages++;
		} else {
			controller->last_result = CG_POC_LAYOUT_RECEIVE_INVALID;
			controller->rejected_messages++;
		}
		break;
	case CG_POC_LAYOUT_RECEIVE_INVALID:
		controller->rejected_messages++;
		break;
	case CG_POC_LAYOUT_RECEIVE_ERROR:
		controller->receive_errors++;
		break;
	}

	return 0;
}

void
cg_poc_layout_controller_init(struct cg_poc_layout_controller *controller)
{
	if (!controller) {
		return;
	}

	cg_poc_layout_socket_init(&controller->socket);
	controller->source = NULL;
	controller->apply_width = NULL;
	controller->data = NULL;
	controller->accepting = false;
	controller->last_result = CG_POC_LAYOUT_RECEIVE_NONE;
	controller->accepted_messages = 0;
	controller->rejected_messages = 0;
	controller->receive_errors = 0;
}

bool
cg_poc_layout_controller_start(struct cg_poc_layout_controller *controller, struct wl_event_loop *event_loop,
			       const char *path, const char *runtime_dir, cg_poc_layout_apply_width_func apply_width,
			       void *data)
{
	if (!controller || !event_loop || !apply_width || controller->source || controller->socket.fd >= 0 ||
	    controller->accepting) {
		errno = EINVAL;
		return false;
	}

	if (!cg_poc_layout_socket_open(&controller->socket, path, runtime_dir)) {
		return false;
	}

	controller->apply_width = apply_width;
	controller->data = data;
	controller->accepting = true;
	controller->last_result = CG_POC_LAYOUT_RECEIVE_NONE;
	controller->accepted_messages = 0;
	controller->rejected_messages = 0;
	controller->receive_errors = 0;
	controller->source = wl_event_loop_add_fd(event_loop, controller->socket.fd, WL_EVENT_READABLE,
						  handle_layout_message, controller);
	if (!controller->source) {
		int saved_errno = errno;
		cg_poc_layout_controller_stop(controller);
		errno = saved_errno;
		return false;
	}

	return true;
}

void
cg_poc_layout_controller_stop(struct cg_poc_layout_controller *controller)
{
	if (!controller) {
		return;
	}

	controller->accepting = false;
	controller->apply_width = NULL;
	controller->data = NULL;
	if (controller->source) {
		wl_event_source_remove(controller->source);
		controller->source = NULL;
	}
	cg_poc_layout_socket_close(&controller->socket);
}
