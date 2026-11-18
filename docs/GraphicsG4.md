# Graphics G4 — water materials

Water shading only, implemented in `shaders/ground.frag`:

- Esil water uses restrained blue-green body tones, broad smooth variation and a slightly warmer shallow-water tint following the existing bank UVs and elliptical end caps.
- Two softly bent directional waves and small texture ripples perturb normals only. Screen-space derivative filtering and the existing G3 mipmapped detail texture suppress distant aliasing. All animation uses the existing scene timer.
- Water reflects the G1 procedural sky through a bounded Fresnel response, receives G2 daylight and restrained warm sun highlights, and retains the existing distant atmosphere.
- The Botanical Garden lake uses calmer ripples and its existing shallow-water palette. Ak Orda basins retain filtered radial ripples with added normal response and sky reflection. Fountain jets are unchanged.
- No new textures, geometry, draw calls, framebuffer passes, transparency sorting or GPU uploads. River level stays at Y=-3.6 m. The complete geometry/rendering source, G1/G2 libraries and G3 materials are unchanged.

Debug x64 build passed. The final shader compiled and rendered the full scene under OpenGL 3.3 on Intel UHD. Matched aerial, embankment, bridge, garden-lake and palace-basin views were inspected. Material flags, texture restoration, depth/blend/offset state and GL error checks passed. Consecutive 1/30-second animation samples changed the river-only pedestrian-view region by at most one 8-bit channel level, with smooth visible motion over one second.

Brief matched 1280×720 hidden-window Debug aerial timing, 45 frames after warm-up: **85.23 → 83.73 FPS**, or **11.73 → 11.94 ms**. This is a short sample subject to timing variation, not a guaranteed visible-window frame rate.

Limitations: reflections approximate sky only; no reflections of buildings/bridge, refraction or simulated water depth. The shoreline tint approximates shallowness without changing the approved river geometry. Stopped for F5 review; no commit or push.
