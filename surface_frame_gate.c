#include "surface_frame_gate.h"

#include <stddef.h>

void
cg_surface_frame_gate_reset(struct cg_surface_frame_gate *gate)
{
	if (!gate) {
		return;
	}
	gate->ready_revision = 0;
	gate->wakeup_revision = 0;
}

struct cg_surface_frame_decision
cg_surface_frame_gate_update(struct cg_surface_frame_gate *gate, uint64_t revision, int actual_width, int actual_height,
			     int expected_width, int expected_height)
{
	struct cg_surface_frame_decision decision = {0};

	if (!gate || revision == 0 || actual_width < 0 || actual_height < 0 || expected_width <= 0 ||
	    expected_height <= 0) {
		cg_surface_frame_gate_reset(gate);
		return decision;
	}
	if (actual_width != expected_width || actual_height != expected_height) {
		gate->ready_revision = 0;
		if (gate->wakeup_revision != revision) {
			gate->wakeup_revision = revision;
			decision.send_wakeup = true;
		}
		return decision;
	}

	decision.ready = true;
	decision.notify_ready = gate->ready_revision != revision;
	gate->ready_revision = revision;
	gate->wakeup_revision = 0;
	return decision;
}
