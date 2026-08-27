#include "surface_frame_gate.h"

#include <assert.h>
#include <stdio.h>

static void
test_exact_buffer_becomes_ready_once(void)
{
	struct cg_surface_frame_gate gate = {0};
	struct cg_surface_frame_decision first = cg_surface_frame_gate_update(&gate, 7, 1320, 800, 1320, 800);
	assert(first.ready);
	assert(first.notify_ready);
	assert(!first.send_wakeup);
	assert(gate.ready_revision == 7);

	struct cg_surface_frame_decision repeated = cg_surface_frame_gate_update(&gate, 7, 1320, 800, 1320, 800);
	assert(repeated.ready);
	assert(!repeated.notify_ready);
	assert(!repeated.send_wakeup);
}

static void
test_mismatch_wakes_once_per_revision(void)
{
	struct cg_surface_frame_gate gate = {0};
	struct cg_surface_frame_decision first = cg_surface_frame_gate_update(&gate, 11, 1024, 768, 1320, 800);
	assert(!first.ready);
	assert(!first.notify_ready);
	assert(first.send_wakeup);
	assert(gate.ready_revision == 0);
	assert(gate.wakeup_revision == 11);

	struct cg_surface_frame_decision repeated = cg_surface_frame_gate_update(&gate, 11, 1024, 768, 1320, 800);
	assert(!repeated.ready);
	assert(!repeated.notify_ready);
	assert(!repeated.send_wakeup);

	struct cg_surface_frame_decision next_revision = cg_surface_frame_gate_update(&gate, 12, 1024, 768, 1320, 800);
	assert(!next_revision.ready);
	assert(next_revision.send_wakeup);
	assert(gate.wakeup_revision == 12);
}

static void
test_mismatch_revokes_readiness_without_repaint_loop(void)
{
	struct cg_surface_frame_gate gate = {0};
	assert(cg_surface_frame_gate_update(&gate, 19, 1440, 900, 1440, 900).ready);
	struct cg_surface_frame_decision mismatch = cg_surface_frame_gate_update(&gate, 19, 1439, 900, 1440, 900);
	assert(!mismatch.ready);
	assert(mismatch.send_wakeup);
	assert(gate.ready_revision == 0);
	assert(!cg_surface_frame_gate_update(&gate, 19, 1439, 900, 1440, 900).send_wakeup);

	cg_surface_frame_gate_reset(&gate);
	assert(gate.ready_revision == 0);
	assert(gate.wakeup_revision == 0);
}

int
main(void)
{
	test_exact_buffer_becomes_ready_once();
	test_mismatch_wakes_once_per_revision();
	test_mismatch_revokes_readiness_without_repaint_loop();
	puts("surface frame gate: PASS");
	return 0;
}
