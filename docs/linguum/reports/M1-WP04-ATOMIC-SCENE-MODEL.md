# M1-WP04 — Generic Atomic Scene Model

Status: PASS (verified unattended)

## Result

Cage now owns a bounded generic scene model aligned with the future
`scene.proto`: scene/output IDs, monotonic revisions, full surface snapshots,
bounds, clip, z-order, visibility, input policy, parent identity, modal state,
focused surface, and validated resize-boundary descriptors for M1-WP05.

The model validates the complete candidate before one assignment commits it.
Stale or conflicting revisions, malformed geometry, duplicate IDs, unknown or
retired registrations, parent mismatches, invalid focus, and invalid boundary
targets leave the previous scene effective. Same-revision semantically identical
snapshots are idempotent even when repeated fields arrive in a different order.

## Live integration

The private seqpacket controller now accepts explicit create, destroy, apply,
and output-resize messages in addition to surface lifecycle messages. A full
snapshot is bounded to 13,360 bytes and uses fixed network-order fields,
reserved-byte checks, exact lengths, and no allocation or partial delta.

Associated views absent from a snapshot, outside the output/clip, hidden, or
non-input are disabled/defocused as required. Visible views receive real
position/size/clip changes. Scene z-order and focus are applied only after the
whole revision commits. Reset, disconnect, and scene destruction erase the
scene and fail closed. Ordinary Cage behavior remains available only when no
framework controller is configured.

Verified implementation: `4627c82ac5e6e5b12bf9af645f70f896dee30238`

```text
pinned Nix Cage build with upstream werror=true: PASS
all focused Meson suites: 10 passed, 0 failed
scene-model suite: PASS
real controller lifecycle/scene dispatch: PASS
framework source-policy test: PASS
clang-format 21.1.8 --dry-run --Werror: PASS
Nix output: /nix/store/2fw2nna0dnsml0cqskhfna3256brnfzf-cage-0.3.0
binary SHA-256: db2d76287df5c3dfca44bd87e68905225f0ea5dfc2bed67137380560212d6f7f
human intervention: none
```

## Automated coverage

- create/destroy idempotence, output uniqueness, and fixed scene capacity;
- stale/conflicting revisions and byte-for-effective-state atomic rollback;
- unordered identical replay, add/update/remove, and full reconnect snapshot;
- pending versus associated input eligibility;
- bounds/output intersection, explicit clip, visibility, input, and z-order;
- output resize deterministic resolution;
- parent/focus/boundary validation and duplicate rejection;
- scene wire golden bytes, truncation, maximum size, reserved bytes, and exact
  create/destroy/apply/resize dispatch through a real Wayland event loop;
- scene reset on controller disconnect and exact scene-wide registry retirement.

No display, media, account, login, clicking, listening, or operator judgment was
used. M1-WP05 may begin.
