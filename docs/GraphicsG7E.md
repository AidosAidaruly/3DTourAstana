# G7E — selectable Japanese anime summer sky

F9 switches between **REALISTIC** (the approved G7D sky) and **ANIME**.
ANIME is the startup default. The existing FPS/movement/F6/F7/F8 title now also
shows `F9 sky ANIME/REALISTIC`, refreshed with the existing title statistics.
Press/hold/release and focus handling match the other graphics toggles.

Anime mode is a separate composition and palette: cobalt/azure upper sky,
cyan-blue lower sky, three enormous vertically developed cumulus formations,
two distant cloud shelves and two smaller scattered clouds. Hero towers have
stacked overlapping billows and broad bases; small clouds use a different lobe
arrangement. Warm-white crowns, blue-gray undersides, soft tonal regions and
delicate trailing wisps produce a deliberately painted appearance. Blue gaps
remain between formations. Highlights derive from the unchanged G7B sun vector.

Cloud shading is computed once from ellipsoid fronts, normals and smooth
depth-weighted color blending, then stored in a 1024² RGBA8 mipmapped angular
field. This adds approximately **5.33 MiB**, alongside the retained 1.33 MiB
G7D field. Both are uploaded at startup; F9 selects the texture and a shader
uniform. Runtime retains three filtered texture samples and the existing one
sky triangle/draw, with no extra render pass, geometry or dependency.
No downloaded texture assets, raymarching or per-frame cloud rebuilding.

Only sky rendering changes. Shared clear-sky/haze and daylight libraries,
sun/shadow direction, materials, water, grading, FXAA, vegetation and city LOD/
instancing remain unchanged. Exact legacy horizon blending prevents a seam
with the existing aerial perspective. Clouds are direction-anchored far-field
art, not moving volumetric weather; they cast no cloud shadows and do not change
building/water reflections or illumination.

Debug x64 build, shaders and OpenGL checks passed. Matched realistic/anime
Baiterek, EXPO, Arena, aerial skyline and upward captures were inspected. The
new shapes and palette are clearly distinct. No obvious horizon seam or tiling
was observed in those views. Small camera shifts and teleport checks passed.
All **16 F6/F7/F8/F9 state combinations** passed. Mode switching performed no
texture uploads and did not invalidate the stationary shadow cache.

Compared with saved G7D captures, realistic mode differed in only 0–9 channel
bytes per image, with maximum channel difference 1/255 (rounding). The original
G7D cache-generation recipe is unchanged.

Brief same-context realistic/anime/anime/realistic check, Debug 1280×720,
Intel UHD, F6/F7/F8 ON: Baiterek **72.88 → 72.56 FPS**, **13.722 → 13.783 ms**.
Sky-heavy samples averaged 10.676 versus 10.260 ms; scheduling/clock variation
prevents claiming a speedup. No substantial regression was observed. NVIDIA
RTX 3050 was not tested. These are short diagnostic samples, not a broad benchmark.

Changed: `SkyAtmosphere.cpp/.h`, `Scene.h`, `main.cpp`, and this document.
Matched PNGs, logs and reproduction harness are ignored under
`obj/graphics-g7e-check/`. Remaining limits: static cached composition, finite
angular-map resolution, no weather animation/parallax or cloud-shadow simulation.
Stopped for F5 style review. No commit or push.
