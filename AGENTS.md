# Linguum Cage Fork Rules

This is the upstream-maintained Cage fork used by the Linguum Firefox
Application Framework. Canonical architecture, source locks, qualification
policy, and milestone reports live in `StevenBuglione/linguum-runtime`.

## Git and upstream

- `origin` is `StevenBuglione/cage`; `upstream` is `cage-kiosk/cage` and must be
  push-disabled.
- `main` tracks upstream development. `linguum` is the published framework
  integration branch. Work-package branches use the `codex/` prefix.
- Never rewrite a published branch or move an upstream/preservation tag.
- Upstream synchronization is separate from framework feature work.

## Scope and architecture

- Cage contains only generic compositor mechanisms. Do not add Linguum app
  titles, provider names, browser-specific widths, or application layout rules.
- Surface identity uses explicit tokens and immutable IDs, never titles.
- Scene revisions apply atomically. Unknown surfaces remain quarantined.
- Resize boundaries are generic scene objects, not browser-divider special cases.
- Test-only diagnostics are capability-scoped and absent from production builds.

## Quality

- Preserve C11, Meson, upstream warnings-as-errors, and `.clang-format`.
- New behavior requires focused deterministic tests and fail-closed validation.
- Qualification is unattended. Missing evidence fails; never ask for a click,
  observation, login, listening check, or approval.
- Run the focused Nix build with the runtime-locked nixpkgs/wlroots environment.
- Never commit build directories, core dumps, credentials, profiles, protected
  pixels, or generated diagnostics.

## Checkpoints

Each verified work-package slice is committed and pushed. Verify local/live
remote SHA equality and update `docs/linguum/` plus the canonical runtime report.
