# Linguum Patch Stack

## Baseline state

M1-WP01 contains no Cage implementation delta from upstream `v0.3.0` commit
`3783af4fadc27057b24025fefe78d942e3f01128`.

## Planned M1 logical series

The qualified POC patch is characterized before replay and split into reviewable
slices:

1. deterministic view-role/layout fixture;
2. private local layout-controller transport;
3. divider hit testing and exclusive pointer grab;
4. fullscreen placement behavior;
5. title-change characterization and removal as identity;
6. teardown characterization and fix;
7. explicit token registry/quarantine;
8. atomic generic scene model;
9. generic ResizeBoundary and events;
10. one-root-AppView reference scene.

Final implementation must contain no application titles, provider names,
browser-specific roles, fixed browser widths, or raw width-message API. This
file is updated with exact commits, upstreamability, tests, and compatibility as
each slice lands.
