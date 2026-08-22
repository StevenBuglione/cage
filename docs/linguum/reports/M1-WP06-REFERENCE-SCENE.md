# M1-WP06 — One-Root-AppView Reference Scene

Status: PASS (verified unattended)

## Result

Cage now runs the reference scene as one full-output trusted AppView plus one
stock Firefox provider surface. The compositor remains the only placement,
clipping, focus, and interactive-resize authority; the React divider is visual
only.

The final Cage head is
`36ebe6a447d2ac9f4ead46a46c04de3269b20970`. It adds two lifecycle properties
required by the live gate:

- scene creation reports the compositor's current output geometry before the
  controller launches either surface;
- teardown signals and waits for the exact primary controller process before
  destroying Wayland clients, while output destruction never re-enables a
  replacement output during server termination.

## Live compositor evidence

```text
initial compositor output: 1600×900
root bounds:               0,0 1600×900
Firefox bounds:            980,42 620×858
surface associations:      2
AppView process count:     1
controls AppView count:    0
```

An unattended guest uinput drag delivered 18 changing events and one commit.
The browser slot became `880,42 720×858`. App Sandbox output transitions to
1440×900 and back to 1600×900 advanced the scene revision while preserving the
720 px committed width and all exact process IDs.

On normal shutdown, the controller recorded `cleanupComplete:true`, systemd
reported result `success`, exit status 0, and restart count 0, and the captured
Cage, AppView, and Firefox PIDs were gone. No core dump was recorded.

## Build evidence

```text
Cage build/tests: PASS
output: /nix/store/nv919vv9sdz49algs0q0kpkz5icnsxdw-cage-0.3.0
binary SHA-256: 5f35bd10d63d574e86a95de870d827efa797d4a171825883412c005b67da63f8
runtime head: c67654a479fc30c5d5412b664b5652fc6a6a693a
Nix/controller head: f02626766094134e70bd8fb070aaf48f5d4e932b
human intervention: none
```

No protected provider pixels, credentials, cookies, or account state were
captured. M1-WP06 passes and M1-WP07 may begin.
