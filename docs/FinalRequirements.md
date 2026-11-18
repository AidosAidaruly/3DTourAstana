# Interactive tour requirements

This is a walkthrough/fly-over project. No missions, enemies, health, scoring or fail screens were added. City geometry, navigation, teleports, material design, LOD/instancing and existing graphics toggles are preserved.

## Controls and identification

- **F1 / H:** readable, translucent help panel. Does not capture camera input.
- **F2:** hide/show all overlays; hidden UI makes no GL calls or texture uploads.
- **P:** pause/resume the scene animation clock. Esil/decorative water, fountain flow/ripples and fountain-jet sway freeze. Static buildings, sky/clouds and vegetation remain static. Camera movement, smooth landing, rendering and wall-clock FPS continue.
- **[ / ]:** sun azimuth -/+15 degrees per press; **R** restores the exact default ray direction `(-0.72, -1, -0.432)`. Elevation stays approximately 50 degrees.
- Existing **F6** shadows, **F7** grading, **F8** FXAA and **F9** cinematic/anime sky remain independent.
- Existing WASD/mouse, F flight, G fast flight, Shift sprint/turbo, Space/Ctrl vertical flight, ESC and teleports **1/2/3/5/6/7/8/9** are retained. There is no teleport on key 4.

The compact HUD identifies **3D Tour to Astana**, team members **Aidos Aidaruly** and **Nurislam Temirgaly**, movement/speed, animation state, sun azimuth, sky mode and effect states. The expanded panel also shows the supplied topic title **3D Tour to Astana**. Official identification is maintained in `src/ProjectInfo.h`; no topic or group number is assumed. SUN SHADOWS labels an implemented primary effect, not a verified topic-sheet requirement.

## Implementation

One cached GDI-rendered Unicode RGBA text panel is drawn after the existing final composite, avoiding grading/FXAA blur on text. It uses the existing Windows/GDI dependency, no UI framework or downloaded font. The panel scales to framebuffer size; text/texture uploads occur only on status/layout changes. UI adds one draw/two triangles when visible, none when hidden.

One 32-byte uniform buffer shares the interactive sunlight direction across all seven atmosphere-aware material/sky programs. Existing light-direction uniforms drive the instanced vertex-lighting paths. Scene shadow preparation uses that same ray and invalidates its cache when the sun changes. Cached cloud composition/lighting rotates with sun azimuth without regenerating cloud textures. No new shadow map, scene pass or city geometry is introduced.

Shader missing-file, compilation and link diagnostics already reached the console through the main exception handler. Empty/read-failed shader files and GLFW initialization/window failures now have explicit diagnostics too. Existing color-grading and shadow FBO completeness checks remain active. Scene assets are procedural; the runtime shader files are the required external assets. Native UI allocation/font/UTF-8 errors also report meaningful messages.

## Validation

- Native Visual Studio 2022 **Debug x64 build passed**. Existing GLFW missing-PDB linker warnings remain; no compilation/link failure.
- Actual OpenGL 3.3 context: **Intel(R) UHD Graphics**, driver `32.0.101.6733`; NVIDIA was not tested.
- Runtime checks passed for all seven sunlight blocks, shader compilation, complete framebuffers, GL errors and state restoration.
- The built application also launched successfully; its title reported the new runtime states, and its error log remained empty. It was then closed normally.
- Checked help/status rendering at **1280x720**, resized **960x540** and **1600x900**. Screenshots inspected for clipping/readability.
- Tested edge/focus handling for F1/H/F2/P/brackets/R; held keys do not repeatedly toggle. Tested the exact camera input code with simulated key states, mouse look, walking/flight/sprint/vertical movement/landing/ESC and all eight teleport coordinates. Native physical keyboard interaction remains part of the user's F5 review.
- Frozen river images remained identical across paused wall time, changed after resume. Sun rotation/reset aligned the visible disc with the CPU light ray; shadow maps updated on rotation and reused stationary results. Reset plus hidden UI restored the same runtime image byte-for-byte.
- All 16 F6/F7/F8/F9 effect combinations rendered without GL errors. These checks exercised the existing effect setters; the application's existing key edge handlers remain unchanged.
- Diagnostic fault injection verified missing/empty shaders, compiler/linker errors and both FBO failure messages. These intentional failures were separate from the successful scene rendering.
- Brief matched Baiterek view, 1280x720 Debug, uncapped, synchronized frames: **HUD hidden 65.9 FPS / 15.16 ms**, **visible 65.6 FPS / 15.25 ms**. Indicative local measurement, not a city-wide benchmark. Hidden overlay does no work; sunlight uploads happen only on adjustments.

## Before submission

1. Provide/check the actual assigned topic sheet. F6-F9 availability alone does not establish topic-specific technique compliance.
2. Perform native F5 review of controls, overlays, pause and sun direction at the presentation display size.
