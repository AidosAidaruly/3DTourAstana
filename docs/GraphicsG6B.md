# Graphics G6B — shadow quality and stability

The existing single **2048×2048 D24** map, caster selection, LOD 2 instancing and light-frustum culling remain. Coverage stays **300×300 m in light space**, approximately **300×347 m on level ground** with the existing sun direction. No geometry, resources, passes or shader variants were added.

- **Filtering:** the narrow four-tap filter becomes a separable 5×5 tent, exactly combined into nine bilinear comparison samples. Fraction-dependent weights keep filtering continuous across texel boundaries. A matched near-ground 10–90% edge transition measured 27 pixels in G6A and 43 pixels in G6B; this is a view-specific softness check, not a universal penumbra width.
- **Bias:** each sample corrects its reference depth to the receiver plane. Minimum receiver bias is 0.015 m instead of 0.025 m; the slope term is 0.04 m times capped tangent slope. Raster offset factor drops from 1.1 to 0.7. These avoid compensating for the wider filter with a large detached-shadow bias.
- **Stability:** fixed projection extent and world-anchored texels remain. The center now advances in eight-texel steps (1.171875 m), reducing small-motion rebuilds without changing the texel lattice. Look-ahead blends continuously into the ground intersection, eliminating G6A's hard downward-pitch threshold. Teleports and changed sun direction still refit immediately.
- **Coverage/appearance:** a rounded superellipse fades the outer corners earlier, retaining the existing axial extent and 30 m axial fade. Occlusion strength is 94%; only direct sunlight changes, retaining the original G2 ambient fill and material colors. Glass, water and other bespoke responses remain unchanged.
- **Invalidation:** unchanged fits reuse the map. `Scene::invalidateShadows()` is an explicit hook for future static caster/content changes. Current casters remain static; no new scene animation or per-frame geometry work was introduced. F6 retains its original behavior and OFF skips depth rendering.

## Verification

Debug x64 build, shader compilation, framebuffer completeness and OpenGL/state checks passed. Matching G6A/G6B building-base, contact, aerial and 3 cm shifted views were inspected. All four matching OFF images are byte-identical. Existing geometry, rendering/LOD/culling code, G1–G5 material files, navigation and project references retain their baseline hashes.

Checks passed for stationary reuse, F6 press/hold/release/focus, integer texel-lattice alignment, explicit invalidation, changed sun direction, and teleporting while OFF then enabling. Forty 2 cm camera steps needed two map updates. Crossing downward direction Y=-0.149 to -0.151 produced no snapped-center jump in the test, replacing the previous hard threshold.

One brief matched dense moving-aerial Debug test, Intel UHD, 1280×720, 45 measured frames after warm-up: **OFF 70.23 FPS / 14.238 ms; ON 67.75 FPS / 14.761 ms**. ON updated the map on 15 of 45 frames; OFF performed zero updates. This short hidden-window measurement is not the user's visible F5 frame rate. Tested updates retained 21 depth draws and 40,300 triangles; cached frames add no depth draws.

Limitations remain: local single-map coverage, fixed-width filtering rather than physically varying contact shadows, and the existing simplified caster silhouettes. Finer architectural details and other landmark renderers do not gain new shadow coverage in this stage.

Stopped for F5 visual review. No commit or push.
