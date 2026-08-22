# M1-WP02 — POC Patch Replay

Status: in progress (checkpoint 1 of 6 verified)

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
