# Linguum Patch Stack

## Baseline state

M1-WP01 contains no Cage implementation delta from upstream `v0.3.0` commit
`3783af4fadc27057b24025fefe78d942e3f01128`.

## M1 logical series

The qualified POC patch is characterized before replay and split into reviewable
slices:

1. deterministic view-role/layout fixture;
2. private local layout-controller transport;
3. divider hit testing and exclusive pointer grab;
4. fullscreen placement behavior;
5. title-change characterization and removal as identity;
6. teardown characterization and fix;
7. explicit token registry/quarantine;
8. atomic generic scene model;
9. generic ResizeBoundary and events;
10. one-root-AppView reference scene.

Final implementation must contain no application titles, provider names,
browser-specific roles, fixed browser widths, or raw width-message API. This
file is updated with exact commits, upstreamability, tests, and compatibility as
each slice lands.

## Verified checkpoints

### 1. Legacy POC layout characterization

- Commits: `95ea4f7b200b389b7878a5efab28a316a7e66817`,
  `864fc3c354b4203cc402ebdb6990b53ed5c976e1`
- Scope: pure, temporary title/width/rectangle fixture and table-driven tests
- Production behavior changed: no
- Pinned Nix build: PASS
- Meson tests: 1 passed, 0 failed
- clang-format 21.1.8: PASS

The `cg_poc_*` namespace and application titles are characterization-only and
must be removed when the token registry and generic scene model replace them.

### 2. Private layout-control socket

- Commits: `cff9471dcddef7cd6ffea38f5d710bb960470061`,
  `0509bb707abda0491214fd643a2873ab410461f1`,
  `af70c8771a0c2c2b68aed9ac518395ad43c8578e`
- Scope: private nonblocking Unix datagram transport, bounded width messages,
  event-loop adapter, and exact cleanup
- Production behavior: matches the POC width-control path when explicitly
  enabled by its environment variables
- Pinned Nix build: PASS
- Meson tests: 2 passed, 0 failed
- clang-format 21.1.8: PASS

The socket integration test proves `0600` permissions, valid delivery,
nonblocking empty receive, rejection of malformed/embedded-NUL/oversized
datagrams, idempotent close, and socket-file removal without a display or human
operator.

### 3. Divider hit testing and pointer grab

- Commits: `1bfdb504cc4cd66c416f0660a96324efe44387f0`,
  `5a2c8ccff400092b53a6eaf343925c701602bb47`
- Scope: exact vertical-divider hit slop, exclusive left-button grab, width
  calculation/clamping, cursor transition, and release
- Pinned Nix build: PASS
- Meson tests: 3 passed, 0 failed
- clang-format 21.1.8: PASS

The pure state-machine suite covers layout origins, both hit-slop edges, output
Y bounds, inactive/duplicate grabs, motion in both directions, fractional
motion truncation, min/max clamping, release, and invalid calls. Generic
ResizeBoundary replaces this temporary vertical-only model in M1-WP05.

### 4. Fullscreen placement

- Commit: `aa383e93ec493e4a8cf1920b64840ac33441a054`
- Scope: fullscreen requests reuse the view's characterized slot placement
  instead of forcing the whole output dimensions
- Pinned Nix build: PASS
- Meson tests: 3 passed, 0 failed
- clang-format 21.1.8: PASS

The layout suite now includes explicit 1920×1080 fullscreen expectations for
workspace, controls, and browser slots, while its disabled-layout cases retain
upstream whole-output behavior.

### 5. Title-change role reassignment

- Commit: `e70bd5f8e731d2977fde50059eaea803b3c98ad7`
- Scope: temporary xdg_toplevel title listener updates the characterized POC
  role/slot and foreign-toplevel title
- Pinned Nix build: PASS
- Meson tests: 3 passed, 0 failed
- clang-format 21.1.8: PASS

The title classification suite includes an explicit transition sequence from
browser to workspace to controls and back to disabled/default. This behavior is
historical accounting only; M1-WP03 removes title changes as identity.

### 6. Controller teardown and event-source cleanup

- Commits: `466bed0cfee0c1ddcfb3a1359d5636ec2b1e1556`,
  `1e6bd42f0511c6b6932a5bae99e890cf4993b140`
- Scope: explicit controller lifecycle, Wayland event-source ownership,
  fail-closed start, accounting counters, and idempotent stop
- Pinned Nix build: PASS
- Meson tests: 4 passed, 0 failed
- clang-format 21.1.8: PASS

The lifecycle suite uses a real Wayland event loop and private Unix socket. It
proves accepted/rejected delivery, callback rejection, double-start refusal,
disable-before-remove teardown, exact socket cleanup, idempotent stop, and clean
display destruction.

All six behaviors from the locked 474-line POC patch are now accounted for.
The temporary `cg_poc_*`, titles, environment variables, and raw-width transport
remain characterization scaffolding—not the framework API—and are replaced in
M1-WP03 through M1-WP05.

### 7. Generic one-time surface registry

- Commits: `d304174409d031ef638b605523340924661b2a7a`,
  `9137a73c894a56dc692668c2dbefdd1e4569ef05`,
  `2d7843ea8f9cbb27ba8358b663526337fc1d91a8`
- Scope: bounded generic registry, 256-bit one-time tokens, expiry, immutable
  association, parent validation, replay protection, retirement, secure reset,
  and versioned framework `app_id` token hints
- Pinned Nix build: PASS
- Meson tests: 5 passed, 0 failed
- clang-format 21.1.8: PASS

The registry test covers invalid/zero fields, duplicate IDs/tokens/surfaces,
unknown/stale/replayed tokens, required popup parent, invalid parent, timeout
bounds/overflow, capacity, retirement, reset token erasure, and token-hint
encoding/decoding. It contains no application, provider, or browser identity.
