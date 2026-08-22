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

## M1-WP02 checkpoint 5 — title-change reassignment

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `e70bd5f8e731d2977fde50059eaea803b3c98ad7` | PASS |
| title transition sequence | layout test | PASS |
| all focused Meson suites | 3 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/ryj6npf72qcaqxkvkhaqw0fbrb454ngw-cage-0.3.0
binary SHA-256: 37ca08d909838299d6965c4c1ef200181e1cfccb615adc14ad987f737360bc0d
```

## M1-WP02 checkpoint 6 — controller teardown

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `1e6bd42f0511c6b6932a5bae99e890cf4993b140` | PASS |
| real Wayland controller lifecycle | Meson test | PASS |
| all focused Meson suites | 4 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/q012d78nqmv7c5sl4s1crml12rak7vwj-cage-0.3.0
binary SHA-256: c5295816fbee51bf9956749a1877ce2fcfa923b47a9cecaab290fa0450d90d8d
```

## M1-WP03 checkpoint 1 — generic surface registry

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `2d7843ea8f9cbb27ba8358b663526337fc1d91a8` | PASS |
| generic surface registry | Meson test | PASS |
| all focused Meson suites | 5 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/7hg8s9mba716ms8jns8qsby95vyfdl39-cage-0.3.0
binary SHA-256: 36adfa29abfd13b4d21633da4a460cdc17073fc44aff1385efa837e087bfb43c
```

## M1-WP03 checkpoint 2 — surface-control protocol

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `96c80f234419648b5ddb6f5533ce87f67ac3c03b` | PASS |
| bounded binary protocol | Meson test | PASS |
| all focused Meson suites | 6 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/rsa4a4fbkxfaay0q8chx4iacxykp9757-cage-0.3.0
binary SHA-256: 7fe7025b7baee6c10a82f50ed2e24dd45afe8bf8434bc861501b17eb3129c709
```

## M1-WP03 checkpoint 3 — surface controller lifecycle

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `29772babf21df5ffd71e9f2f40543bd551c730a5` | PASS |
| real seqpacket/Wayland controller | Meson test | PASS |
| all focused Meson suites | 7 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/p296ffw8kkwxip13n3mch317dli5n6wk-cage-0.3.0
binary SHA-256: 90aa81107c0691619d9689c2535bd900c7f7b88a0649cc74a2f376bdaf9ea874
```

## M1-WP03 checkpoint 4 — live association and quarantine

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `4c7b4ba3b4b9d2f4604551e9150f3ab61fdab36c` | PASS |
| immutable one-time view association | policy + production integration | PASS |
| explicit association event | golden vector + real seqpacket delivery | PASS |
| unknown/stale/replayed quarantine | policy suite | PASS |
| reset/unregister/disconnect invalidation | lifecycle + generation tests | PASS |
| no title/raw-socket production identity | source-policy test | PASS |
| all focused Meson suites | 9 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/fm5s25j0qdxpmswk7nry1a6kr8b43f52-cage-0.3.0
binary SHA-256: ae8e7119af8d5956eb8e4ae833bbd558a90fdd9c0d9381b87dfa6fe2fdf3c2a6
```

## M1-WP04 — generic atomic scene model

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `4627c82ac5e6e5b12bf9af645f70f896dee30238` | PASS |
| full snapshot atomicity/revisions | scene-model suite | PASS |
| bounds/clip/z-order/visibility/input | scene-model + live integration build | PASS |
| pending/associated/reconnect/output resize | scene-model suite | PASS |
| bounded scene controller messages | protocol + real event-loop suite | PASS |
| production source policy | source-policy test | PASS |
| all focused Meson suites | 10 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/2fw2nna0dnsml0cqskhfna3256brnfzf-cage-0.3.0
binary SHA-256: db2d76287df5c3dfca44bd87e68905225f0ea5dfc2bed67137380560212d6f7f
```

## M1-WP05 — generic compositor resize boundary

| Component | Exact version/source | Result |
|---|---|---|
| Cage source | `782c7f6372d5f6acbc1984e861fa0e79bcef046a` | PASS |
| four-edge model, hit test, min/max | resize-boundary suite | PASS |
| high-rate throttle and cancellation | resize-boundary suite | PASS |
| fixed lifecycle event protocol | golden vector + real seqpacket delivery | PASS |
| live pointer/scene integration | production wlroots build | PASS |
| no POC layout/resize in executable | source-policy test | PASS |
| all focused Meson suites | 11 passed, 0 failed | PASS |
| source formatting | clang-format `21.1.8` | PASS |
| warnings | upstream `werror=true` | PASS |
| human intervention | none | PASS |

Output:

```text
/nix/store/004y6cmq60m7a09yz6vj2bqkqp4c8fin-cage-0.3.0
binary SHA-256: 8328858898080e358ccbe53419666eb7a43e17eabd8c206c1df3e7e7275e10ae
```
