# M1 Final Hard Gate

Status: PASS (verified unattended)

The final Cage source is
`f2d4005b0d2b7c10bb7481e63b07f2cf1dad0609`. The last hard-gate audit moved
the opaque token hint from a framework-owned name to the Cage-owned
`cage-surface-v1` namespace. Production Cage C/H source contains no Linguum,
provider, browser-width, workspace, or controls constant.

The imported appliance pins this exact commit as a non-flake input and applies
zero inline patches.

```text
focused Cage/Nix tests: PASS
Cage package: /nix/store/rix5kgigw4g3a9aw5w4x2dlwzr7ck2j1-cage-linguum-0.3.0-linguum-f2d4005b0d2b
raw Cage binary SHA-256: 3cd9d580fa49f82e9016cd0c088520b92e75aec911d5337368f5d5104c317171
reference-scene check: /nix/store/83rh9isjh0c0blawwalyb95nvrdfw2zg-linguum-reference-scene-check
source-mapping check: /nix/store/2pqvmvc9lijzhmh0riwg61602445v8xx-linguum-m1-appliance-integration-check
kiosk system closure: /nix/store/ysbw5svq641nhjps70gs64x64vb5hjzr-nixos-system-linguum-26.05.20260820.5880666
inline Cage patches: 0
```

The exact Cage binary, matching AppView/controller, and updated stress harness
then passed a five-cycle live canary and the complete 50-cycle gate:

```text
real divider commits: 1000
bounded changing events: 4000
surface lifecycles: 100
distinct token sets: 50
unique exact process sets: 50
per-cycle cleanup: 50 / 50
new core dumps: 0
normal App Sandbox shutdown: PASS
human intervention: false
```

The retained final lifecycle result SHA-256 is
`f814532186c29460061e8d52506fd26cd52805657b086e0859d5208e2e92f713`.
No prior core was deleted or waived, and no protected pixels, credentials,
account state, provider DOM, or media traffic were captured.
