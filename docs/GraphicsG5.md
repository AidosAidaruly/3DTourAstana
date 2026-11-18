# Graphics G5 — architectural materials

- Shared `BuildingMaterials.h` GLSL provides a tinted sky/horizon/ground reflection gradient, Fresnel response and bounded sun highlights. Main opaque architectural glazing and instanced residential glazing share this model; medium/far residential LODs retain vertex-evaluated, interpolated reflections.
- Existing residential glass quads receive deterministic pane tint, occasional darker panes and a gentle vertical coating gradient through their existing vertex-color attribute, once during initialization. Pane identity excludes recess depth, keeping it consistent across LODs. No positions, normals, indices, attributes or mesh sizes change.
- Existing background material batches explicitly distinguish mineral facades, matte roofs, brick and architectural metal from site furniture/vegetation. Stone/concrete and roofs reuse the mipmapped G3 detail texture for restrained tonal/grain variation; near brick uses coverage-filtered mortar. Premium brick and Ak Orda facade stone receive the same mineral finish. Architectural metal has a modest reflected sheen; roofs remain matte.
- Northern Lights retains turquoise coatings, spandrels, seals and silver bands, with filtered pane variation and the shared environment gradient. Abu Dhabi, Nur Alem, Talan and Khan tower glazing retain their own pane patterns, coating colors, framing and reflection weights while blending in the improved environment. MEGA/EXPO canopy glazing receives a restrained environment adjustment with unchanged alpha, seals and transparent draw/sorting behavior. Other bespoke roof, gold, stadium and greenhouse effects remain intact.

## Verification and cost

Final Debug x64 build passed. Full-scene shader compilation, material resets, depth/blend/offset state, texture restoration and GL error checks passed on Intel UHD. Matched Northern Lights, residential, street-window, skyline, EXPO and MEGA captures were inspected. An additional close residential comparison used the saved pre-G5 background shader/colors; a clear Talan view was also inspected.

Source comparisons verify unchanged geometry generation, placements, vertex layouts, render/LOD/culling functions, G1/G2 libraries, G3 ground and G4 water code. Sampled sky and grass regions in the matching aerial frames are byte-identical. Existing glass alpha expressions remain unchanged.

**0 added triangles, 0 added draw calls, 0 added textures or per-frame uploads.** Shared buffers and material batching remain. Brief matched hidden-window Debug aerial timing, 1280×720, same camera/GPU, 45 measured frames after warm-up: **61.89 → 61.35 FPS**, approximately **16.16 → 16.30 ms**. This short sample is subject to normal timing variation.

Reflections remain an analytic environment approximation, with no captured buildings, real-time cubemaps, interiors or new reflection passes. Fine mineral detail deliberately fades through mip filtering. Stopped for F5 review; no commit or push.
