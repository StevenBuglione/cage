# M1-WP01 — Cage Fork and Unmodified Baseline

**Result:** PASS
**Date:** 2026-08-22
**Repository:** `StevenBuglione/cage`
**Branch:** `codex/M1-WP01-cage-fork`
**Base:** `3783af4fadc27057b24025fefe78d942e3f01128`

## Fork and history

- GitHub reports a real fork of `cage-kiosk/cage`.
- Clone is full history (`is-shallow-repository=false`) with all tags.
- `origin` fetch/push targets the fork.
- `upstream` fetches the original and is push-disabled.
- fork default `main` tracks upstream's current default history.
- `linguum` and this work branch begin at exact upstream `v0.3.0`.
- upstream tag object `12df8389263d756f63ed234dbb44daf550127986`
  peels to `3783af4fadc27057b24025fefe78d942e3f01128`.

## Unmodified build

The clean source was synchronized by full SHA to the persistent `venus`
worktree. Nix evaluated the exact appliance-pinned nixpkgs and overrode only the
derivation source path, with an empty patch list.

```text
source SHA: 3783af4fadc27057b24025fefe78d942e3f01128
NixOS source: 08419b8465f8a525ce9bf47acd4101c34b76fb7b
nixpkgs: 5880666fd9eb563038431edb35c2d0aa595884e6
project: Cage 0.3.0
wlroots: 0.20.0
Xwayland: true
warnings-as-errors: enabled
output: /nix/store/xz1yl4iwwiskhir45g90wl4c7y90xk8l-cage-0.3.0
binary SHA-256: bb7caa08a4bc50c3e1e5e7cdaf4078548234b00fad9af4c98a320ebf7db8089d
result: PASS
```

## Completion assertions

- [x] Exact upstream source was derived from pinned nixpkgs, not guessed.
- [x] Full history, fork relation, tags, remotes, and branches are preserved.
- [x] Upstream push is disabled.
- [x] Untouched source builds with the qualified dependency versions.
- [x] License and binary output are recorded.
- [x] No POC patch or app-specific logic is present.
- [x] Qualification required no human intervention.
