#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "poc_layout_socket.h"

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

int
main(void)
{
	char runtime_template[] = "/tmp/cage-poc-layout-XXXXXX";
	char path[108];
	char oversized[32];
	const char embedded_null[] = {'6', '2', '0', '\0', '7'};
	struct cg_poc_layout_socket socket;
	struct stat metadata;
	int width = 0;
	const char *runtime_dir = mkdtemp(runtime_template);

	assert(runtime_dir);
	assert(snprintf(path, sizeof(path), "%s/layout.sock", runtime_dir) > 0);
	cg_poc_layout_socket_init(&socket);
	assert(socket.fd == -1);
	assert(cg_poc_layout_socket_open(&socket, path, runtime_dir));
	assert(socket.fd >= 0);
	assert(stat(path, &metadata) == 0);
	assert((metadata.st_mode & 0777) == 0600);
	assert(cg_poc_layout_socket_receive(&socket, &width) == CG_POC_LAYOUT_RECEIVE_NONE);

	send_message(path, "620", 3);
	assert(cg_poc_layout_socket_receive(&socket, &width) == CG_POC_LAYOUT_RECEIVE_WIDTH);
	assert(width == 620);

	send_message(path, "620px", 5);
	assert(cg_poc_layout_socket_receive(&socket, &width) == CG_POC_LAYOUT_RECEIVE_INVALID);
	send_message(path, embedded_null, sizeof(embedded_null));
	assert(cg_poc_layout_socket_receive(&socket, &width) == CG_POC_LAYOUT_RECEIVE_INVALID);
	memset(oversized, '6', sizeof(oversized));
	send_message(path, oversized, sizeof(oversized));
	assert(cg_poc_layout_socket_receive(&socket, &width) == CG_POC_LAYOUT_RECEIVE_INVALID);

	cg_poc_layout_socket_close(&socket);
	assert(socket.fd == -1);
	assert(access(path, F_OK) < 0);
	cg_poc_layout_socket_close(&socket);
	assert(rmdir(runtime_dir) == 0);
	return 0;
}
