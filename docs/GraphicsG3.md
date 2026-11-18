# Graphics G3 — ground materials

- Main terrain and grassy Esil bank tops use continuous, restrained green variation instead of the one-metre checkerboard. EXPO's existing meadow blend and riverbank soil/elevation transitions remain.
- Grass, asphalt and generic stone share one static 256×256 RGBA8 packed noise texture. Three seamless smooth-noise channels provide broad/detail variation; the fourth supplies fine grain. Repeat wrapping and trilinear mip filtering suppress distant grain aliasing. Full mip storage is approximately 0.33 MiB.
- Asphalt receives subtle tonal variation and aggregate grain, retaining G2's matte lighting. Markings are separate, unchanged batches.
- Explicitly selected city-road stone and background residential paving use 1.6×0.8 m staggered slabs, softly filtered 7 mm joints and restrained per-stone variation. Existing Abu Dhabi sidewalk textures retain their joints and gain only grain/tone variation. Other textured landmark paving is untouched.
- Building, glass, metal, vehicle, seat and water shading is unchanged. G1 sky/haze and G2 lighting calculations are unchanged. Geometry, placements, render/LOD/culling logic and draw counts are unchanged.
- Texture unit 1 is bound for the scene and restored afterward; existing unit-0 material textures remain independent. Sampler assignments and texture creation happen at initialization, with no per-frame uploads.

## Validation

Debug x64 solution build passed. Full-scene OpenGL 3.3 rendering passed five matching before/after views: aerial grass/Ak Orda, a central street junction, Baiterek plaza, Esil/bridge entrance and Botanical Garden/Arena road. Captures confirm removal of the terrain/bank checkerboards and preservation of landmark paving and markings. Shader compilation, material resets, depth/blend/offset states, texture binding restoration and GL error checks passed.

Brief matched hidden-window aerial timing, same camera/build/Intel UHD GPU at 1280×720: **87.99 → 103.64 FPS** (11.36 → 9.65 ms), 45 measured frames after warm-up. This short sample establishes no observed regression, not a reliable speedup estimate or a visible-window F5 guarantee.

Cost: **0 added triangles, 0 added draw calls**, one small static detail texture. Fine texture intentionally fades with distance; this stage adds no displacement, grass blades or new geometry. Stopped for F5 review; no commit or push.
