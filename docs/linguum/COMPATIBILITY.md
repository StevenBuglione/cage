# Compatibility Matrix

## M1-WP01 unmodified baseline

| Component | Exact version/source | Result |
|---|---|---|
| Cage | `v0.3.0` / `3783af4fadc27057b24025fefe78d942e3f01128` | PASS |
| nixpkgs | `5880666fd9eb563038431edb35c2d0aa595884e6` | PASS |
| wlroots | `0.20.0` | PASS |
| Meson | `1.10.2` | PASS |
| GCC | `15.2.0` | PASS |
| Nix builder | `venus`, x86_64, 32 logical cores | PASS |
| Xwayland build | enabled | PASS |
| license | MIT | PASS |

Output:

```text
/nix/store/xz1yl4iwwiskhir45g90wl4c7y90xk8l-cage-0.3.0
binary SHA-256: bb7caa08a4bc50c3e1e5e7cdaf4078548234b00fad9af4c98a320ebf7db8089d
```

The build used the exact clean source worktree and the appliance's pinned
nixpkgs dependency graph. Upstream Meson `werror=true` remained enabled. No
source patch, current-main substitution, interactive display, or human action
was involved.

## M1-WP02 checkpoint 1 — layout characterization

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `864fc3c354b4203cc402ebdb6990b53ed5c976e1` | PASS |
| legacy POC layout test | Meson test, 1 case suite | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/kmr8lpy4wscn4szg6f8kfqv26n5p7gwc-cage-0.3.0
binary SHA-256: 42bcf73a490927024ecf67af24957dceef7c1d4a02c785832ee2777784fe3d6f
```

## M1-WP02 checkpoint 2 — layout control socket

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `af70c8771a0c2c2b68aed9ac518395ad43c8578e` | PASS |
| layout characterization | Meson test | PASS |
| real Unix socket integration | Meson test | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/nlklzjqmkbbd3c6jghz8j1zp50x0hzc2-cage-0.3.0
binary SHA-256: 9fc86cfb008b6f7688d05833200938db08be3994071ef45cce443142b21ff2d3
```

## M1-WP02 checkpoint 3 — divider pointer grab

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `5a2c8ccff400092b53a6eaf343925c701602bb47` | PASS |
| layout characterization | Meson test | PASS |
| real Unix socket integration | Meson test | PASS |
| divider state machine | Meson test | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/rg7vxkgx8gvlzxaqrjw35ylj95rcm977-cage-0.3.0
binary SHA-256: 31e499579b02866b3530c585e77ff69bea03edb04e72ec40a3a7da3cbcb07138
```

## M1-WP02 checkpoint 4 — fullscreen placement

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `aa383e93ec493e4a8cf1920b64840ac33441a054` | PASS |
| fullscreen slot expectations | layout test | PASS |
| all focused Meson suites | 3 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/y47kmx8g9gx9l8641mlgmwb28pc9q3r8-cage-0.3.0
binary SHA-256: 0d3a30a6fa1a6eec554d6fe7967a59a55b94c4474257c9f871ae9aad64dd2719
```
