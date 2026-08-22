# Upstream Policy

## Repository identity

```text
fork:     https://github.com/StevenBuglione/cage
upstream: https://github.com/cage-kiosk/cage
origin default branch: main
upstream default branch: master
framework integration branch: linguum
```

The GitHub repository retains the native fork relationship. The local
`upstream` remote fetches from the original project and has push URL `DISABLED`.

## Qualified baseline

The Linguum appliance pins NixOS source
`08419b8465f8a525ce9bf47acd4101c34b76fb7b`, which pins nixpkgs
`5880666fd9eb563038431edb35c2d0aa595884e6`. That nixpkgs derivation selects
Cage `v0.3.0` and wlroots 0.20.

```text
upstream tag object: 12df8389263d756f63ed234dbb44daf550127986
peeled source commit: 3783af4fadc27057b24025fefe78d942e3f01128
license: MIT
```

`linguum` begins at the peeled commit. The POC patch is replayed as reviewed
logical commits after an unmodified baseline build; it is never committed as an
opaque permanent patch file here.

## Sync procedure

1. Fetch full history and tags from both remotes.
2. Create `upstream-sync/<date>` from the current `main`.
3. Record old/new upstream commits and release/toolchain changes.
4. Update `main` without rewriting published history.
5. Rebase no published Linguum branch; merge or replay through a dedicated PR.
6. Run upstream build/tests and the complete applicable Linguum regression gate.
7. Update compatibility and patch-stack records in a separate sync PR.

Generic fixes should be proposed upstream where practical. Linguum framework
protocol/scene features remain isolated on `linguum` until upstream accepts an
appropriate generic form.
