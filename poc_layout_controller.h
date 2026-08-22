#ifndef CG_POC_LAYOUT_CONTROLLER_H
#define CG_POC_LAYOUT_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>

#include "poc_layout_socket.h"

typedef bool (*cg_poc_layout_apply_width_func)(void *data, int width);

struct cg_poc_layout_controller {
	struct cg_poc_layout_socket socket;
	struct wl_event_source *source;
	cg_poc_layout_apply_width_func apply_width;
	void *data;
	bool accepting;
	enum cg_poc_layout_receive_result last_result;
	uint64_t accepted_messages;
	uint64_t rejected_messages;
	uint64_t receive_errors;
};

void cg_poc_layout_controller_init(struct cg_poc_layout_controller *controller);
bool cg_poc_layout_controller_start(struct cg_poc_layout_controller *controller, struct wl_event_loop *event_loop,
				    const char *path, const char *runtime_dir,
				    cg_poc_layout_apply_width_func apply_width, void *data);
void cg_poc_layout_controller_stop(struct cg_poc_layout_controller *controller);

#endif
