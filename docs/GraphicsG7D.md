# G7D — anime-inspired daylight clouds

Replaced the sparse noise-threshold cloud appearance with composed cumulus
clusters: rounded overlapping puffs, scalloped feathered edges, subtle interior
density variation, cooler undersides and restrained sun-facing highlights.
Blue openings remain between clusters, with a small sky-only zenith refinement.
The shared clear-sky/haze and G7B daylight libraries are unchanged.

Ten unequal cloud groups are generated once into a 512×512 RGBA8 angular field,
with premultiplied cloud color/coverage and mipmaps (~1.33 MiB). This small
procedural cache avoids evaluating cloud lobes every frame; there is no external
texture asset. Two samples from the existing G3 noise map soften/paint the field:
three runtime texture samples total, as before. The existing single sky triangle
and draw/pass count are unchanged. No raymarching, simulation or new geometry.

Cloud coordinates are direction-anchored, with no repeated dome tiling or camera
translation swimming. The generated highlights use the same sun constant as the
scene/shadows. Texture bindings and sky GL state are restored; the cache is deleted
with the sky. Existing materials, water, controls, shadows, grading, FXAA, LOD,
instancing and vegetation are untouched. Clouds are static and do not cast shadows.

Debug x64 and full-scene shader/OpenGL checks passed. Matched Baiterek street,
EXPO/Nur Alem, Arena, high skyline and upward sky-heavy captures were inspected.
The cloud change is clearly visible; nearby architecture stays crisp. No obvious
banding, tiling or repeated texture pattern was seen in those views. All eight
F6/F7/F8 state combinations passed.

Brief nearest matched old/new samples, Debug 1280×720, Intel UHD, effects ON:
Baiterek 73.10 → 72.84 FPS (13.680 → 13.729 ms); sky-heavy 95.75 → 94.23 FPS
(10.444 → 10.613 ms). Approximately +0.05 / +0.17 ms in these short samples;
not a broad benchmark or NVIDIA result. Earlier samples showed clock/timing drift.

Changed only `SkyAtmosphere.cpp/.h` and this document. Before/after PNGs, logs
and reproduction harnesses are ignored under `obj/graphics-g7d-check/`.
No commit or push. Stopped for F5 style review.
