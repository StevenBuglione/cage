/*
 * Cage: A Wayland kiosk.
 *
 * Private connection-oriented controller for pending surface registration.
 *
 * See the LICENSE file accompanying this file.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "surface_controller.h"

static bool
socket_path_valid(const char *path, const char *runtime_dir, size_t path_capacity)
{
	size_t runtime_length;

	if (!path || !*path || !runtime_dir || !*runtime_dir || path_capacity == 0) {
		return false;
	}
	runtime_length = strlen(runtime_dir);
	if (runtime_dir[0] != '/' || runtime_dir[runtime_length - 1] == '/' || path[0] != '/' ||
	    strlen(path) >= path_capacity) {
		return false;
	}
	return strncmp(path, runtime_dir, runtime_length) == 0 && path[runtime_length] == '/' &&
	       path[runtime_length + 1] != '\0';
}

static bool
set_descriptor_flags(int fd)
{
	int descriptor_flags = fcntl(fd, F_GETFD);
	int status_flags = fcntl(fd, F_GETFL);

	return descriptor_flags >= 0 && status_flags >= 0 && fcntl(fd, F_SETFD, descriptor_flags | FD_CLOEXEC) >= 0 &&
	       fcntl(fd, F_SETFL, status_flags | O_NONBLOCK) >= 0;
}

static uint64_t
monotonic_now(void *data)
{
	struct timespec timestamp;

	if (clock_gettime(CLOCK_MONOTONIC, &timestamp) < 0) {
		return 0;
	}
	return (uint64_t) timestamp.tv_sec * 1000 + (uint64_t) timestamp.tv_nsec / 1000000;
}

static void
emit_event(struct cg_surface_controller *controller, enum cg_surface_controller_event_type type, cg_scene_id scene_id,
	   cg_surface_id surface_id, cg_scene_revision revision)
{
	const struct cg_surface_controller_event event = {
		.type = type,
		.scene_id = scene_id,
		.surface_id = surface_id,
		.revision = revision,
	};

	if (controller->event) {
		controller->event(&event, controller->event_data);
	}
}

static void
disconnect_client(struct cg_surface_controller *controller)
{
	if (controller->client_source) {
		wl_event_source_remove(controller->client_source);
		controller->client_source = NULL;
	}
	if (controller->client_fd >= 0) {
		close(controller->client_fd);
		controller->client_fd = -1;
	}
	if (controller->registry) {
		cg_surface_registry_reset(controller->registry);
	}
	if (controller->scenes) {
		cg_scene_model_reset(controller->scenes);
	}
	emit_event(controller, CG_SURFACE_CONTROLLER_RESET, 0, 0, 0);
	controller->disconnects++;
}

static bool
send_registered(struct cg_surface_controller *controller, const struct cg_surface_registration_request *request)
{
	uint8_t bytes[CG_SURFACE_CONTROL_REGISTERED_SIZE];
	size_t size;
	ssize_t sent;

	if (!controller || controller->client_fd < 0 ||
	    !cg_surface_control_encode_registered(request, bytes, sizeof(bytes), &size)) {
		return false;
	}
	sent = send(controller->client_fd, bytes, size, MSG_NOSIGNAL);
	if (sent == (ssize_t) size) {
		return true;
	}
	controller->receive_errors++;
	disconnect_client(controller);
	return false;
}

static void
apply_message(struct cg_surface_controller *controller, const struct cg_surface_control_message *message)
{
	bool applied = false;

	controller->last_registry_result = CG_SURFACE_REGISTRY_INVALID;
	controller->last_scene_result = CG_SCENE_INVALID;
	switch (message->type) {
	case CG_SURFACE_CONTROL_REGISTER:
		controller->last_registry_result = cg_surface_registry_register(
			controller->registry, &message->registration, controller->now(controller->now_data));
		applied = controller->last_registry_result == CG_SURFACE_REGISTRY_OK;
		if (applied) {
			applied = send_registered(controller, &message->registration);
		}
		break;
	case CG_SURFACE_CONTROL_UNREGISTER:
		controller->last_registry_result = cg_surface_registry_retire(
			controller->registry, message->unregistration.scene_id, message->unregistration.surface_id);
		if (controller->last_registry_result == CG_SURFACE_REGISTRY_OK) {
			emit_event(controller, CG_SURFACE_CONTROLLER_RETIRED, message->unregistration.scene_id,
				   message->unregistration.surface_id, 0);
		}
		applied = controller->last_registry_result == CG_SURFACE_REGISTRY_OK;
		break;
	case CG_SURFACE_CONTROL_RESET:
		cg_surface_registry_reset(controller->registry);
		cg_scene_model_reset(controller->scenes);
		controller->last_registry_result = CG_SURFACE_REGISTRY_OK;
		controller->last_scene_result = CG_SCENE_OK;
		emit_event(controller, CG_SURFACE_CONTROLLER_RESET, 0, 0, 0);
		applied = true;
		break;
	case CG_SURFACE_CONTROL_CREATE_SCENE:
		controller->last_scene_result = cg_scene_model_create(
			controller->scenes, message->create_scene.scene_id, message->create_scene.output_id,
			message->create_scene.output_width, message->create_scene.output_height);
		applied = controller->last_scene_result == CG_SCENE_OK;
		if (applied) {
			emit_event(controller, CG_SURFACE_CONTROLLER_SCENE_CREATED, message->create_scene.scene_id, 0,
				   0);
		}
		break;
	case CG_SURFACE_CONTROL_DESTROY_SCENE:
		controller->last_scene_result =
			cg_scene_model_destroy(controller->scenes, message->destroy_scene.scene_id);
		if (controller->last_scene_result == CG_SCENE_OK) {
			(void) cg_surface_registry_retire_scene(controller->registry, message->destroy_scene.scene_id);
			emit_event(controller, CG_SURFACE_CONTROLLER_SCENE_DESTROYED, message->destroy_scene.scene_id,
				   0, 0);
			applied = true;
		}
		break;
	case CG_SURFACE_CONTROL_APPLY_SCENE:
		controller->last_scene_result =
			cg_scene_model_apply(controller->scenes, controller->registry, &message->scene_snapshot);
		if (controller->last_scene_result == CG_SCENE_OK) {
			emit_event(controller, CG_SURFACE_CONTROLLER_SCENE_APPLIED, message->scene_snapshot.scene_id, 0,
				   message->scene_snapshot.revision);
			applied = true;
		}
		break;
	case CG_SURFACE_CONTROL_RESIZE_OUTPUT: {
		const struct cg_scene_record *record =
			cg_scene_model_find(controller->scenes, message->resize_output.scene_id);
		if (!record || record->snapshot.output_id != message->resize_output.output_id) {
			controller->last_scene_result = record ? CG_SCENE_INVALID : CG_SCENE_NOT_FOUND;
			break;
		}
		controller->last_scene_result = cg_scene_model_resize_output(
			controller->scenes, message->resize_output.scene_id, message->resize_output.output_width,
			message->resize_output.output_height);
		if (controller->last_scene_result == CG_SCENE_OK) {
			emit_event(controller, CG_SURFACE_CONTROLLER_OUTPUT_RESIZED, message->resize_output.scene_id, 0,
				   record->snapshot.revision);
			applied = true;
		}
		break;
	}
	case CG_SURFACE_CONTROL_REGISTERED:
	case CG_SURFACE_CONTROL_ASSOCIATED:
	case CG_SURFACE_CONTROL_BOUNDS_CHANGING:
	case CG_SURFACE_CONTROL_BOUNDS_COMMITTED:
	case CG_SURFACE_CONTROL_RESIZE_CANCELLED:
	case CG_SURFACE_CONTROL_OUTPUT_CHANGED:
		break;
	}
	if (applied) {
		controller->applied_messages++;
	} else {
		controller->rejected_messages++;
	}
}

static void
receive_message(struct cg_surface_controller *controller)
{
	uint8_t bytes[CG_SURFACE_CONTROL_MAX_MESSAGE_SIZE];
	struct cg_surface_control_message message;
	ssize_t size = recv(controller->client_fd, bytes, sizeof(bytes), MSG_TRUNC);

	if (size == 0) {
		disconnect_client(controller);
		return;
	}
	if (size < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			controller->receive_errors++;
			disconnect_client(controller);
		}
		return;
	}
	if ((size_t) size > sizeof(bytes)) {
		controller->last_parse_result = CG_SURFACE_CONTROL_PARSE_INVALID_SIZE;
		controller->rejected_messages++;
		return;
	}

	controller->last_parse_result = cg_surface_control_parse(bytes, (size_t) size, &message);
	if (controller->last_parse_result != CG_SURFACE_CONTROL_PARSE_OK) {
		controller->rejected_messages++;
		return;
	}
	apply_message(controller, &message);
}

static int
handle_client(int fd, uint32_t mask, void *data)
{
	struct cg_surface_controller *controller = data;

	if (!controller->accepting) {
		return 0;
	}
	if (mask & WL_EVENT_READABLE) {
		receive_message(controller);
	}
	if (controller->client_fd >= 0 && (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR))) {
		disconnect_client(controller);
	}
	return 0;
}

static bool
peer_is_current_user(int fd)
{
	struct ucred credentials;
	socklen_t size = sizeof(credentials);

	return getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &size) == 0 && size == sizeof(credentials) &&
	       credentials.uid == geteuid();
}

static int
handle_listener(int fd, uint32_t mask, void *data)
{
	struct cg_surface_controller *controller = data;
	int client_fd;

	if (!controller->accepting || !(mask & WL_EVENT_READABLE)) {
		return 0;
	}
	client_fd = accept(fd, NULL, NULL);
	if (client_fd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			controller->receive_errors++;
		}
		return 0;
	}
	if (controller->client_fd >= 0 || !set_descriptor_flags(client_fd) || !peer_is_current_user(client_fd)) {
		controller->rejected_connections++;
		close(client_fd);
		return 0;
	}

	controller->client_fd = client_fd;
	controller->client_source =
		wl_event_loop_add_fd(controller->event_loop, client_fd,
				     WL_EVENT_READABLE | WL_EVENT_HANGUP | WL_EVENT_ERROR, handle_client, controller);
	if (!controller->client_source) {
		close(controller->client_fd);
		controller->client_fd = -1;
		controller->receive_errors++;
	}
	return 0;
}

void
cg_surface_controller_init(struct cg_surface_controller *controller)
{
	if (!controller) {
		return;
	}
	memset(controller, 0, sizeof(*controller));
	controller->listener_fd = -1;
	controller->client_fd = -1;
	controller->last_parse_result = CG_SURFACE_CONTROL_PARSE_OK;
	controller->last_registry_result = CG_SURFACE_REGISTRY_OK;
	controller->last_scene_result = CG_SCENE_OK;
}

bool
cg_surface_controller_start(struct cg_surface_controller *controller, struct wl_event_loop *event_loop,
			    const char *path, const char *runtime_dir, struct cg_surface_registry *registry,
			    struct cg_scene_model *scenes, cg_surface_controller_now_func now, void *now_data,
			    cg_surface_controller_event_func event, void *event_data)
{
	struct sockaddr_un address = {.sun_family = AF_UNIX};

	if (!controller || !event_loop || !registry || !scenes || controller->accepting ||
	    controller->listener_fd >= 0 || !socket_path_valid(path, runtime_dir, sizeof(address.sun_path))) {
		errno = EINVAL;
		return false;
	}
	controller->listener_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (controller->listener_fd < 0 || !set_descriptor_flags(controller->listener_fd)) {
		goto fail;
	}
	strcpy(address.sun_path, path);
	unlink(path);
	if (bind(controller->listener_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
		goto fail;
	}
	strcpy(controller->socket_path, path);
	if (chmod(path, 0600) < 0 || listen(controller->listener_fd, 1) < 0) {
		goto fail;
	}
	controller->event_loop = event_loop;
	controller->registry = registry;
	controller->scenes = scenes;
	controller->now = now ? now : monotonic_now;
	controller->now_data = now_data;
	controller->event = event;
	controller->event_data = event_data;
	controller->accepting = true;
	controller->listener_source = wl_event_loop_add_fd(event_loop, controller->listener_fd, WL_EVENT_READABLE,
							   handle_listener, controller);
	if (!controller->listener_source) {
		goto fail;
	}
	return true;

fail: {
	int saved_errno = errno;
	cg_surface_controller_stop(controller);
	errno = saved_errno;
	return false;
}
}

uint64_t
cg_surface_controller_now(const struct cg_surface_controller *controller)
{
	return controller && controller->now ? controller->now(controller->now_data) : 0;
}

bool
cg_surface_controller_notify_associated(struct cg_surface_controller *controller,
					const struct cg_surface_identity *identity)
{
	uint8_t bytes[CG_SURFACE_CONTROL_ASSOCIATED_SIZE];
	size_t size;
	ssize_t sent;

	if (!controller || !controller->accepting || controller->client_fd < 0 ||
	    !cg_surface_control_encode_associated(identity, bytes, sizeof(bytes), &size)) {
		return false;
	}
	sent = send(controller->client_fd, bytes, size, MSG_NOSIGNAL);
	if (sent == (ssize_t) size) {
		return true;
	}
	controller->receive_errors++;
	disconnect_client(controller);
	return false;
}

bool
cg_surface_controller_notify_resize(struct cg_surface_controller *controller, const struct cg_resize_event *event)
{
	struct cg_surface_controller_event controller_event;
	uint8_t bytes[CG_SURFACE_CONTROL_BOUNDS_EVENT_SIZE];
	size_t size;
	ssize_t sent;

	if (!controller || !controller->accepting || controller->client_fd < 0 ||
	    !cg_surface_control_encode_resize_event(event, bytes, sizeof(bytes), &size)) {
		return false;
	}
	sent = send(controller->client_fd, bytes, size, MSG_NOSIGNAL);
	if (sent != (ssize_t) size) {
		controller->receive_errors++;
		disconnect_client(controller);
		return false;
	}
	memset(&controller_event, 0, sizeof(controller_event));
	controller_event.scene_id = event->scene_id;
	controller_event.surface_id = event->surface_id;
	controller_event.revision = event->revision;
	controller_event.boundary_id = event->boundary_id;
	controller_event.bounds = event->bounds;
	switch (event->type) {
	case CG_RESIZE_EVENT_BOUNDS_CHANGING:
		controller_event.type = CG_SURFACE_CONTROLLER_BOUNDS_CHANGING;
		break;
	case CG_RESIZE_EVENT_BOUNDS_COMMITTED:
		controller_event.type = CG_SURFACE_CONTROLLER_BOUNDS_COMMITTED;
		break;
	case CG_RESIZE_EVENT_CANCELLED:
		controller_event.type = CG_SURFACE_CONTROLLER_RESIZE_CANCELLED;
		break;
	case CG_RESIZE_EVENT_NONE:
		return false;
	}
	if (controller->event) {
		controller->event(&controller_event, controller->event_data);
	}
	return true;
}

bool
cg_surface_controller_notify_output_changed(struct cg_surface_controller *controller,
					    const struct cg_surface_control_resize_output *event)
{
	uint8_t bytes[CG_SURFACE_CONTROL_RESIZE_OUTPUT_SIZE];
	size_t size;
	ssize_t sent;

	if (!controller || !controller->accepting || controller->client_fd < 0 ||
	    !cg_surface_control_encode_output_changed(event, bytes, sizeof(bytes), &size)) {
		return false;
	}
	sent = send(controller->client_fd, bytes, size, MSG_NOSIGNAL);
	if (sent == (ssize_t) size) {
		return true;
	}
	controller->receive_errors++;
	disconnect_client(controller);
	return false;
}

void
cg_surface_controller_stop(struct cg_surface_controller *controller)
{
	if (!controller) {
		return;
	}
	controller->accepting = false;
	if (controller->client_source || controller->client_fd >= 0) {
		disconnect_client(controller);
	} else if (controller->registry) {
		cg_surface_registry_reset(controller->registry);
		cg_scene_model_reset(controller->scenes);
		emit_event(controller, CG_SURFACE_CONTROLLER_RESET, 0, 0, 0);
	}
	if (controller->listener_source) {
		wl_event_source_remove(controller->listener_source);
		controller->listener_source = NULL;
	}
	if (controller->listener_fd >= 0) {
		close(controller->listener_fd);
		controller->listener_fd = -1;
	}
	if (controller->socket_path[0]) {
		unlink(controller->socket_path);
		controller->socket_path[0] = '\0';
	}
	controller->registry = NULL;
	controller->scenes = NULL;
	controller->event_loop = NULL;
	controller->now = NULL;
	controller->now_data = NULL;
	controller->event = NULL;
	controller->event_data = NULL;
}
