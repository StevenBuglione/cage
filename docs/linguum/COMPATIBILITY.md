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
