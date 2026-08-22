# M1-WP05 — Generic Compositor Resize Boundary

Status: PASS (verified unattended)

## Result

Cage now owns application-agnostic resize boundaries as part of the atomic
scene snapshot. Each bounded descriptor carries a stable boundary ID, target
surface ID, any of four edges, minimum and maximum size, hit slop, row/column
cursor, enabled state, and visible state.

The compositor performs deterministic z-ordered hit testing only against
visible associated targets. Pressing a boundary enters an exclusive pointer
session: client pointer delivery is suspended, geometry is applied directly to
the real scene surface, and bounded lifecycle events are returned to the
controller. `BoundsChanging` is throttled to one message per 16 ms,
`BoundsCommitted` is exact, and `ResizeCancelled` restores the last committed
bounds whenever the active scene revision still exists.

The session cancels on focus loss, target hide/replacement/retirement/unmap,
controller reset or disconnect, scene destruction, output resize, pointer
removal, and compositor teardown. The boundary cursor cannot be replaced by a
client while the exclusive interaction is active.

Verified implementation: `782c7f6372d5f6acbc1984e861fa0e79bcef046a`

```text
pinned Nix Cage build with upstream werror=true: PASS
all focused Meson suites: 11 passed, 0 failed
generic resize-boundary suite: PASS
bounded event golden vector and real seqpacket delivery: PASS
framework source-policy test: PASS
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/004y6cmq60m7a09yz6vj2bqkqp4c8fin-cage-0.3.0
binary SHA-256: 8328858898080e358ccbe53419666eb7a43e17eabd8c206c1df3e7e7275e10ae
human intervention: none
```

## Automated coverage

- left, right, top, and bottom edges;
- minimum/maximum clamping and opposite-edge anchoring;
- enabled, visible, target-visible, and association eligibility;
- high-rate pointer movement and 16 ms controller-event throttling;
- commit and rollback-bearing cancellation;
- focus loss, target retirement/hide/revision replacement, and output resize;
- fixed-size network-order event framing and rejection as inbound commands;
- real Wayland event-loop delivery of a committed-bounds event;
- production compilation with the generic state machine and without the POC
  layout/resize source files.

The historical vertical divider and browser width code remain test-only
characterization fixtures. The Cage executable contains no fixed browser
width, title role, raw width socket, or `cg_poc_resize` dependency.

No display, media, account, login, clicking, listening, or operator judgment
was used. M1-WP06 may begin.
