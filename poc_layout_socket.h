#ifndef CG_POC_LAYOUT_SOCKET_H
#define CG_POC_LAYOUT_SOCKET_H

#include <stdbool.h>

struct cg_poc_layout_socket {
	int fd;
	char path[108];
};

enum cg_poc_layout_receive_result {
	CG_POC_LAYOUT_RECEIVE_NONE,
	CG_POC_LAYOUT_RECEIVE_WIDTH,
	CG_POC_LAYOUT_RECEIVE_INVALID,
	CG_POC_LAYOUT_RECEIVE_ERROR,
};

void cg_poc_layout_socket_init(struct cg_poc_layout_socket *layout_socket);
bool cg_poc_layout_socket_open(
	struct cg_poc_layout_socket *layout_socket, const char *path, const char *runtime_dir);
enum cg_poc_layout_receive_result cg_poc_layout_socket_receive(
	struct cg_poc_layout_socket *layout_socket, int *width_out);
void cg_poc_layout_socket_close(struct cg_poc_layout_socket *layout_socket);

#endif
