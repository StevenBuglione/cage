# M1-WP03 — Explicit Surface Registry

Status: in progress (checkpoint 1 verified)

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

## Remaining gate

- bounded explicit controller protocol;
- controller-driven pending registration and cleanup;
- live view association using `app_id` hint;
- disabled/quarantined unknown views and no focus;
- removal of title-derived production identity and raw-width production control.
