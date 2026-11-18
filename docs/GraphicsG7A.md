# Graphics G7A — Sky 2.0 and atmosphere

Implemented in `src/SkyAtmosphere.cpp` only; city geometry, G2 lighting colors, G3–G5 material code, G6 shadows/F6, controls, LOD, instancing and caches are unchanged.

- A richer blue zenith blends through lighter blue to a soft horizon. Restrained sunward warmth and two smooth aureole scales follow the existing sunlight direction. No solar bloom, exposure change or post-processing was added.
- Thin cloud patches and wisps use three filtered lookups of the existing G3 packed procedural-noise texture. Direction-anchored coordinates, soft thresholds and mip filtering avoid camera-translation swimming and hard noise. Cloud detail is evaluated only in the sky shader, not on every material.
- Shared optical-depth haze begins after 120 m and increases gently, with less veiling when looking down. Its color comes from the same directional clear sky, avoiding a separate gray horizon/fog wall. Nearby surfaces remain crisp. Existing water reflection automatically follows the new clear-sky palette through its original `daySky()` call; its material code is untouched.

Debug x64 build and full-scene shader/OpenGL checks passed. Five identical before/after cameras were inspected: elevated city overview, Baiterek street, Esil skyline, a sunward sky-rich view, and EXPO. The gradient/cloud changes are clearly visible in matched views. Framebuffer/viewport/depth/blend/offset and texture binding checks, F6 input handling and stationary shadow caching passed. The matched close-pavement region is byte-identical; changes remain in the environment/distance presentation.

The existing sky triangle, draw order and GL state restoration remain unchanged. **No new textures, geometry, draw calls or rendering passes.** The sky reuses texture unit 1 already scoped by Scene's `GroundMaterials::Binding`; its sampler is assigned once at initialization. Clear-sky normalization is shared and close geometry skips haze work.

Brief matched hidden-window Debug overview, Intel UHD, 1280×720, F6 ON, same camera and 45 measured frames after warm-up: **96.97 → 98.58 FPS**, **10.313 → 10.144 ms**. No regression was observed; this short sample does not establish a reliable speed gain or guaranteed visible-window frame rate.

Clouds are static, distant shader layers, not volumetric weather; they cast no cloud shadows and are not captured in building reflections. No changes were made outside the atmosphere implementation and this document. Stopped for F5 review; no commit or push.
