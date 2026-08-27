#ifndef CG_SURFACE_FRAME_GATE_H
#define CG_SURFACE_FRAME_GATE_H

#include <stdbool.h>
#include <stdint.h>

struct cg_surface_frame_gate {
	uint64_t ready_revision;
	uint64_t wakeup_revision;
};

struct cg_surface_frame_decision {
	bool ready;
	bool notify_ready;
	bool send_wakeup;
};

void cg_surface_frame_gate_reset(struct cg_surface_frame_gate *gate);

struct cg_surface_frame_decision cg_surface_frame_gate_update(struct cg_surface_frame_gate *gate, uint64_t revision,
							      int actual_width, int actual_height, int expected_width,
							      int expected_height);

#endif
