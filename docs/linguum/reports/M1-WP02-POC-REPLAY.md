# M1-WP02 — POC Patch Replay

Result: PASS (6 of 6 checkpoints verified)

## Locked artifact

- POC patch SHA-256: `8e5aff551b75e3711052955d76e47ddf22e7aba6f4c323163e5210cd35e64c6d`
- Length: 474 lines
- Diff: 7 files, 290 insertions, 5 deletions
- Baseline: Cage `3783af4fadc27057b24025fefe78d942e3f01128`
- Apply check: PASS

## Checkpoint 1 — layout characterization

The original patch's parsing, title-prefix classification, narrow-output
fallback, controls height, output-origin handling, and minimum target dimensions
are captured in a pure local fixture. The fixture is not wired into production
Cage behavior and is explicitly temporary.

Verified Cage head: `864fc3c354b4203cc402ebdb6990b53ed5c976e1`

```text
local GCC -std=c11 -Wall -Wextra -Werror -Wundef: PASS
local fixture execution: PASS
pinned Nix Cage build: PASS
Meson poc-layout-characterization: 1 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/kmr8lpy4wscn4szg6f8kfqv26n5p7gwc-cage-0.3.0
binary SHA-256: 42bcf73a490927024ecf67af24957dceef7c1d4a02c785832ee2777784fe3d6f
```

All verification ran unattended on clean source. No display, protected content,
account data, pixels, watching, clicking, or listening was used.

## Checkpoint 2 — private layout-control socket

The production event-loop adapter now uses a small testable transport boundary.
It accepts only a socket beneath `XDG_RUNTIME_DIR`, sets close-on-exec and
nonblocking flags, applies `0600`, and rejects empty, malformed, embedded-NUL,
and oversized datagrams. Teardown is idempotent and removes the exact socket.

Verified Cage head: `af70c8771a0c2c2b68aed9ac518395ad43c8578e`

```text
pinned Nix Cage build: PASS
Meson poc-layout-characterization: PASS
Meson poc-layout-socket: PASS
Meson total: 2 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/nlklzjqmkbbd3c6jghz8j1zp50x0hzc2-cage-0.3.0
binary SHA-256: 9fc86cfb008b6f7688d05833200938db08be3994071ef45cce443142b21ff2d3
```

The integration test creates and drives a real Unix datagram socket inside a
temporary private runtime directory. It requires no compositor display or
operator and cleans its exact resources before exiting.

## Checkpoint 3 — divider hit testing and pointer grab

The original divider behavior is represented as a pure temporary state machine
and a thin seat adapter. Tests lock the exact hit slop, output-coordinate bounds,
exclusive left-button grab, fractional motion truncation, width direction,
minimum/maximum clamps, and release behavior.

Verified Cage head: `5a2c8ccff400092b53a6eaf343925c701602bb47`

```text
pinned Nix Cage build: PASS
Meson poc-layout-characterization: PASS
Meson poc-layout-socket: PASS
Meson poc-resize-characterization: PASS
Meson total: 3 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/rg7vxkgx8gvlzxaqrjw35ylj95rcm977-cage-0.3.0
binary SHA-256: 31e499579b02866b3530c585e77ff69bea03edb04e72ec40a3a7da3cbcb07138
```

The tests run without a compositor display and require no operator input.

## Checkpoint 4 — fullscreen placement

Fullscreen requests now reuse `view_position`, preserving the same
characterized surface slot instead of overriding it with whole-output
dimensions. The layout suite contains explicit 1920×1080 fullscreen slot
expectations and retains disabled-layout fallback coverage.

Verified Cage head: `aa383e93ec493e4a8cf1920b64840ac33441a054`

```text
pinned Nix Cage build: PASS
all focused Meson suites: 3 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/y47kmx8g9gx9l8641mlgmwb28pc9q3r8-cage-0.3.0
binary SHA-256: 0d3a30a6fa1a6eec554d6fe7967a59a55b94c4474257c9f871ae9aad64dd2719
```

No display or human observation was used.

## Checkpoint 5 — title-change role reassignment

A temporary xdg_toplevel title listener now reproduces the POC's role and slot
update and keeps the foreign-toplevel title current. The pure suite proves the
browser → workspace → controls → disabled/default transition sequence.

Verified Cage head: `e70bd5f8e731d2977fde50059eaea803b3c98ad7`

```text
pinned Nix Cage build: PASS
all focused Meson suites: 3 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/ryj6npf72qcaqxkvkhaqw0fbrb454ngw-cage-0.3.0
binary SHA-256: 37ca08d909838299d6965c4c1ef200181e1cfccb615adc14ad987f737360bc0d
```

This listener exists only for behavioral accounting. The explicit registry in
M1-WP03 makes identity immutable across title changes.

## Checkpoint 6 — controller teardown and event-source cleanup

The POC controller now owns its socket and Wayland event source explicitly. Its
stop path disables new requests before removing the source, closes and unlinks
the exact socket, clears callbacks, and is idempotent. Counters expose accepted,
rejected, and receive-error outcomes without inspecting account or media data.

Verified Cage head: `1e6bd42f0511c6b6932a5bae99e890cf4993b140`

```text
pinned Nix Cage build: PASS
poc-layout-characterization: PASS
poc-layout-socket: PASS
poc-layout-controller-lifecycle: PASS
poc-resize-characterization: PASS
Meson total: 4 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/q012d78nqmv7c5sl4s1crml12rak7vwj-cage-0.3.0
binary SHA-256: c5295816fbee51bf9956749a1877ce2fcfa923b47a9cecaab290fa0450d90d8d
```

The lifecycle test drives a real Wayland event loop and private socket. It
proves valid delivery, malformed and callback rejection accounting,
double-start refusal, exact cleanup, repeated stop, and clean display teardown.

## Locked patch accounting

| Original file/behavior | Verified replacement |
|---|---|
| `view.c` / three-surface role and rectangles | pure `poc_layout` fixture plus thin view adapter |
| `cage.c` / runtime socket and width message | bounded socket and controller modules |
| `seat.c` / divider grab | pure `poc_resize` state machine plus thin seat adapter |
| `xdg_shell.c` / fullscreen slot | role-aware `view_position` path |
| `xdg_shell.c/.h` / title update | temporary title listener and transition test |
| `server.h` and `cage.c` / teardown ownership | explicit idempotent controller lifecycle |

The locked patch SHA-256 remains
`8e5aff551b75e3711052955d76e47ddf22e7aba6f4c323163e5210cd35e64c6d`;
it applies cleanly to the exact Cage v0.3.0 baseline. No behavior remains
unclassified.

## Work-package gate

- [x] All six logical behaviors replayed and characterized.
- [x] Deterministic three-surface rectangle fixture exists.
- [x] Real socket and Wayland event-loop lifecycles are tested.
- [x] Pinned wlroots/Cage build passes with warnings as errors.
- [x] All focused tests and source formatting pass.
- [x] Every implementation and evidence checkpoint is pushed.
- [x] Verification is fully unattended and uses no protected/user data.
- [x] App-specific constructs are explicitly temporary and prohibited as the final API.

M1-WP03 may begin. Its first hard requirement is replacing title-based identity
with the explicit token registry and quarantine model.
