# M1-WP07 — Lifecycle and Teardown

Status: PASS (verified unattended)

## Result

Cage head `62b47ca3f4984984086ca112ddf451bc6c0d9a01` passed the
generic-scene lifecycle gate. Its focused registry test performs 100 complete
register, associate, and retire cycles. Existing tests also cover controller
disconnect, active-resize cancellation, invalid ownership, and server
termination order.

The live gate used Nix/controller head
`e82a3fef370cb23897d4d698df30f7a3cd973da7` and runtime head
`356937f2635dfccf2b98783863f8b1f93a0e022f`.

## Live compositor evidence

```text
fresh compositor starts:       50
real divider drags per start:   20
committed divider drags:        1000
bounded changing events:        4000
surface associations retired:   100
distinct token sets:            50
unique Cage PID sets:           50
per-cycle cleanup receipts:     50 / 50
systemd success results:        50 / 50
automatic restarts:             0
new core dumps:                 0
human intervention:             false
```

After each stop, the exact Cage, controller, AppView, and Firefox parent PIDs
were absent. A separate exact-controller crash probe exercised service recovery
without process-name matching, and its replacement stopped cleanly.

The first full attempt exposed retained WebKitGTK/Mesa helper-process cores
during AppView teardown. The AppView shutdown handshake was corrected in the
Nix repository and the exact rebuilt binary subsequently passed a 10-cycle
canary plus two consecutive 50-cycle gates. No dump was waived or removed.

## Build evidence

```text
Cage build/tests: PASS
output: /nix/store/cg0rxkvwca373jk08srw043mdh0xfqhd-cage-0.3.0
binary SHA-256: 5f35bd10d63d574e86a95de870d827efa797d4a171825883412c005b67da63f8
Nix focused check: /nix/store/3piibpvmb56ba2k4ikvf828r2spzbfbm-linguum-reference-scene-check
result artifact SHA-256: f97c8d89da4e584f9512aa89df2e48f591a35ab689d3939ee4a37441d53e6a4b
normal App Sandbox VM shutdown: PASS
```

No protected pixels, credentials, cookies, media traffic, account actions, or
manual observations were used. M1-WP07 passes and M1-WP08 may begin.
