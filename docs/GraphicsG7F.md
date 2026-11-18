# G7F — natural cinematic sky and visible sun

**CINEMATIC** is now the default presentation. F9 still switches to the preserved
G7E ANIME mode; the existing window title displays the selected mode. F6/F7/F8,
navigation, all geometry/materials and the shared G7B daylight remain unchanged.

Natural mode replaces stacked lobes with asymmetric multi-scale cloud density,
irregular erosion, billowy height shading, soft self-occlusion and thin wispy
edges. Large and medium broken cloud masses have bright tops and cooler gray
undersides. This is generated once into a 1024² mipmapped premultiplied RGBA8
field, rather than evaluating cloud construction during every frame. It is a
cached far-field approximation, not volumetric weather simulation.

A softly antialiased ~0.53-degree sun disc and two-scale warm halo use the exact
existing sun direction (approximately 50-degree elevation). They are composited
before cloud coverage, so dense cloud regions can obscure the solar image.
There is no bloom framebuffer/pass, lens flare or additional directional light.
The sky-only zenith refinement and sun effect do not alter scene lighting,
reflections, aerial-perspective uniforms, shadow strength or material colors.

Rendering retains three filtered cloud/noise samples, one sky triangle/draw and
the existing pass count. Both modes reuse their startup textures without runtime
uploads. The natural field grows from G7D's 512² to 1024²: approximately **4 MiB**
additional mipmapped payload; both mode fields total approximately 10.67 MiB.
No downloaded assets, dependencies or per-frame CPU geometry work.

Debug x64 build, shaders and OpenGL checks passed. Seven matched mode views cover
Baiterek, EXPO/Nur Alem, Arena, skyline, upward sky, the sun direction and a
sunlit Baiterek angle. The sun-direction center sampled RGB 254/245/225 with
grading enabled. The inspected views showed no obvious horizon seams or tiling;
nearby architecture remained unchanged. F9 default/press/hold/focus, all 16
F6/F7/F8/F9 combinations, shadow-cache reuse, shifts/teleport and no-runtime-
texture-upload checks passed. Preserved Anime captures differed by at most 1/255
in one channel, consistent with shader rounding.

Brief same-context cinematic/anime/anime/cinematic sampling, Debug 1280×720,
Intel UHD, F6/F7/F8 ON: street **13.59 vs 13.32 ms** (~73.6 vs 75.1 FPS);
sunward sky **9.58 vs 9.25 ms** (~104.4 vs 108.1 FPS). Approximately +0.3 ms in
these short samples; clock/scheduling variation limits precision. RTX 3050 was
not tested. No draw-call increase.

Changed only `SkyAtmosphere.cpp/.h` and this document. Shared haze/daylight
libraries and the Anime cache recipe were checked unchanged. Matched PNGs,
logs and reproduction harness are ignored under `obj/graphics-g7f-check/`.
Limits: static composition, finite angular-map resolution, approximate cached
cloud depth, no moving weather/cloud shadows and no update to city reflections.
Stopped for F5 review. No commit or push.
