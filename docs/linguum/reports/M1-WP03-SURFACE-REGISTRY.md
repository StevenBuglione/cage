# M1-WP03 — Explicit Surface Registry

Status: in progress (checkpoints 1–3 verified)

## Checkpoint 1 — pure registry and token hint

The compositor-private registry now models nonzero `SceneId` and `SurfaceId`,
generic surface kinds, optional parents, a 256-bit token, pending/associated/
expired/retired states, a monotonic deadline, and an opaque associated surface.

Verified Cage head: `2d7843ea8f9cbb27ba8358b663526337fc1d91a8`

```text
local GCC warnings-as-errors registry suite: PASS
pinned Nix Cage build: PASS
surface-registry Meson suite: PASS
all focused Meson suites: 5 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/7hg8s9mba716ms8jns8qsby95vyfdl39-cage-0.3.0
binary SHA-256: 36adfa29abfd13b4d21633da4a460cdc17073fc44aff1385efa837e087bfb43c
```

Tested fail-closed cases:

- invalid and zero IDs, tokens, kinds, parents, and timeouts;
- duplicate IDs, tokens, and opaque surface identities;
- unknown, expired, retired, and replayed tokens;
- required popup parent and cross-entry parent validation;
- fixed registry capacity and deadline overflow;
- permanent association and retirement semantics;
- secure reset of all token and registry storage;
- versioned app-id token hint round trip, prefix, length, hex, and zero token.

The token hint is framework-controlled metadata, not a page title. No web,
account, media, protected-pixel, display, or human input was used.

## Checkpoint 2 — bounded explicit controller protocol

The temporary M1 protocol carries explicit register, unregister, and reset
messages. Its fixed header includes magic, version, type, and exact length. The
register body contains network-byte-order scene/surface/parent IDs, kind,
parent-present flag, timeout, reserved bytes, and a 32-byte token. Maximum size
is 72 bytes.

Verified Cage head: `96c80f234419648b5ddb6f5533ce87f67ac3c03b`

```text
local GCC warnings-as-errors protocol suite: PASS
pinned Nix Cage build: PASS
surface-control-protocol Meson suite: PASS
all focused Meson suites: 6 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/rsa4a4fbkxfaay0q8chx4iacxykp9757-cage-0.3.0
binary SHA-256: 7fe7025b7baee6c10a82f50ed2e24dd45afe8bf8434bc861501b17eb3129c709
```

The test locks golden network-order bytes and rejects truncation at every
length, oversized/trailing bytes, declared-length mismatch, bad magic/version/
type, reserved bits, zero or inconsistent fields, and undersized encoders.

## Checkpoint 3 — private surface controller lifecycle

Production Cage now uses a single-peer, same-user Unix `SOCK_SEQPACKET`
controller at `CAGE_FRAMEWORK_CONTROL_SOCKET`. It feeds only validated protocol
messages to the registry. Packet boundaries make oversize rejection exact, and
connection teardown makes pending-token cleanup observable and automatic.

Verified Cage head: `29772babf21df5ffd71e9f2f40543bd551c730a5`

```text
pinned Nix Cage build: PASS
surface-controller-lifecycle Meson suite: PASS
all focused Meson suites: 7 passed, 0 failed
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/p296ffw8kkwxip13n3mch317dli5n6wk-cage-0.3.0
binary SHA-256: 90aa81107c0691619d9689c2535bd900c7f7b88a0649cc74a2f376bdaf9ea874
```

The real event-loop test proves `0600` socket ownership, one peer, valid
register/unregister/reset, duplicate and malformed rejection, oversized packet
rejection, disconnect token erasure, reconnect, exact unlink, and idempotent
stop. The raw width controller is no longer part of the Cage executable.

## Remaining gate

- bounded explicit controller protocol;
- controller-driven pending registration and cleanup;
- live view association using `app_id` hint;
- disabled/quarantined unknown views and no focus;
- removal of title-derived production identity and raw-width production control.
