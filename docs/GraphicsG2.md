# Graphics G2 - shared daytime material lighting

Adds shared `dayDiffuseLight` and `dayMetalHighlight` GLSL functions in
SkyAtmosphere.cpp. The G1 sky/haze functions, sky renderer, daylight direction
and .42/.60 scene uniforms are unchanged. The lighting library is injected
only into geometry fragment programs, never the sky program.

Opaque surfaces receive restrained warm directional sunlight and a normal-based
hemisphere: cool, brighter sky above and softer warm ground bounce below.
A small diffuse-energy shoulder prevents white clipping. Stone, brick,
asphalt, vegetation and paving remain matte. Explicit metal tags select a
bounded roughness-controlled highlight; no color-based material guessing.
Existing background/vehicle material attributes carry those tags without
new vertex streams, geometry, instance buffers, draw calls or texture assets.

Glass diffuse/reflection/Fresnel/alpha calculations are preserved. Existing
water, membrane and ivory-scale effects retain their specialized response.
Silver landmark finishes and stadium seating keep calibrated local fill,
using the same shared diffuse calculation. No shadow maps, multi-light loops,
post-processing, real-time reflections or additional passes are introduced.

Debug x64 and full-scene shader/OpenGL checks pass. Identical residential,
elevated city, Arena entrance, EXPO and MEGA views were inspected for exposure,
contrast and glass. G1 sky source and sampled sky pixels are byte-identical.
The brief matched hidden Debug 1280x720 Intel UHD sample measured 76.87 ->
78.66 FPS (13.01 -> 12.71 ms); variation is too large to claim a speed gain.
Layout, geometry generators, caches, render/LOD functions, camera controls
and unrelated working-tree edits are preserved. Cast shadows remain outside G2.

Stopped for F5 review; no commit or push. Artifacts are ignored in
obj/graphics-g2-check.
