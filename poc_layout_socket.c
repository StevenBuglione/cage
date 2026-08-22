/*
 * Cage: A Wayland kiosk.
 *
 * Temporary, testable transport for the original Linguum POC layout patch.
 * The typed controller transport replaces this file in M1-WP03/M2.
 *
 * See the LICENSE file accompanying this file.
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "poc_layout.h"
#include "poc_layout_socket.h"

void
cg_poc_layout_socket_init(struct cg_poc_layout_socket *layout_socket)
{
	if (!layout_socket) {
		return;
	}

	layout_socket->fd = -1;
	layout_socket->path[0] = '\0';
}

bool
cg_poc_layout_socket_open(struct cg_poc_layout_socket *layout_socket, const char *path, const char *runtime_dir)
{
	struct sockaddr_un address = {.sun_family = AF_UNIX};
	int descriptor_flags;
	int status_flags;

	if (!layout_socket || layout_socket->fd >= 0 ||
	    !cg_poc_layout_socket_path_valid(path, runtime_dir, sizeof(address.sun_path))) {
		errno = EINVAL;
		return false;
	}

	layout_socket->fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (layout_socket->fd < 0) {
		return false;
	}

	descriptor_flags = fcntl(layout_socket->fd, F_GETFD);
	status_flags = fcntl(layout_socket->fd, F_GETFL);
	if (descriptor_flags < 0 || status_flags < 0 ||
	    fcntl(layout_socket->fd, F_SETFD, descriptor_flags | FD_CLOEXEC) < 0 ||
	    fcntl(layout_socket->fd, F_SETFL, status_flags | O_NONBLOCK) < 0) {
		goto fail;
	}

	strcpy(address.sun_path, path);
	unlink(path);
	if (bind(layout_socket->fd, (struct sockaddr *) &address, sizeof(address)) < 0 || chmod(path, 0600) < 0) {
		goto fail;
	}

	strcpy(layout_socket->path, path);
	return true;

fail:
	{
		int saved_errno = errno;
		close(layout_socket->fd);
		layout_socket->fd = -1;
		unlink(path);
		errno = saved_errno;
		return false;
	}
}

enum cg_poc_layout_receive_result
cg_poc_layout_socket_receive(struct cg_poc_layout_socket *layout_socket, int *width_out)
{
	char message[32];
	ssize_t size;

	if (!layout_socket || layout_socket->fd < 0 || !width_out) {
		errno = EINVAL;
		return CG_POC_LAYOUT_RECEIVE_ERROR;
	}

	size = recv(layout_socket->fd, message, sizeof(message) - 1, MSG_TRUNC);
	if (size < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return CG_POC_LAYOUT_RECEIVE_NONE;
		}
		return CG_POC_LAYOUT_RECEIVE_ERROR;
	}

	if (!cg_poc_layout_parse_message(message, (size_t) size, width_out)) {
		return CG_POC_LAYOUT_RECEIVE_INVALID;
	}

	return CG_POC_LAYOUT_RECEIVE_WIDTH;
}

void
cg_poc_layout_socket_close(struct cg_poc_layout_socket *layout_socket)
{
	if (!layout_socket) {
		return;
	}

	if (layout_socket->fd >= 0) {
		close(layout_socket->fd);
		layout_socket->fd = -1;
	}
	if (layout_socket->path[0]) {
		unlink(layout_socket->path);
		layout_socket->path[0] = '\0';
	}
}
