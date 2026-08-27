# FR-WP04 first-frame wake-up

## Result

The compositor's first-frame gate now keeps an associated but incorrectly
sized client productive without revealing it. A scene revision may issue one
direct frame callback while the client buffer does not match the requested
physical bounds. The callback is bounded to one per revision, so it cannot
become an idle repaint loop. The scene node remains disabled until the client
commits an exact-size buffer and the existing first-frame receipt succeeds.

This removes the deadlock where a hidden WebKitGTK surface could wait for a
frame callback while Cage waited for that surface to commit its configured
size. It does not relax the exact-size presentation gate.

## Invariants

- An incorrectly sized buffer is never presented and never accepts input.
- A repeated mismatch in one scene revision cannot request another wake-up.
- A new revision may request one new wake-up for its new bounds.
- A matching buffer produces one readiness notification per revision.
- Hiding, quarantine, unmap, and initialization reset frame-gate state.

## Verification

- `surface-frame-gate` deterministic state-machine test: PASS.
- Pinned Linguum Nix/wlroots compilation with warnings as errors: PASS.
- Full Windows AppView first-frame and resize qualification remains owned by
  the canonical Linguum runtime checkpoint.
