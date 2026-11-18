# Graphics G1 - daytime sky and atmosphere

`SkyAtmosphere.cpp/.h` owns a static OpenGL 3.3 sky program and an empty
VAO. One fullscreen triangle draws before the city, at far depth without
writing depth. Program, VAO, depth function/write mask and depth/blend/cull/
polygon-offset enables are restored. No texture, framebuffer, post-process,
shadow map, animation or per-frame geometry/buffer upload is introduced.

The sky blends a pale, relatively warm horizon through blue lower sky to
a clear upper sky, with a broad restrained aureole aligned to the existing
daylight direction. Static sub-byte dithering prevents gradient banding.
The gradient continues slightly below the geometric horizon for elevated
views. Existing terrain distance-fade colors now meet that same sky.

One shared GLSL library adds aerial perspective after every material branch
in the city, background-building, vehicle and stadium-seat programs. The
composition happens only at shader compilation. Glass alpha is untouched.
Haze starts beyond 180 m, reaches about 7% at 500 m, and is capped at 24%;
downward aerial sightlines use a reduced 16% cap. Nearby materials stay clear.
Scene ambient/diffuse strengths change from .35/.65 to .42/.60 for modest
daylight fill; the sun direction and all material palettes remain unchanged.

Debug x64 and full-scene shader/OpenGL checks pass. Elevated city, pedestrian,
EXPO and MEGA glass views were inspected. A separate sky probe verifies the
gradient, untouched depth and restoration from non-default GL states.
An isolated same-context sky/haze toggle at 1280x720 Debug on Intel UHD gave
77.45 FPS off / 76.39 FPS on (about +0.18 ms); this short sample is not a
visible F5 performance guarantee. Separate-process timings varied.

All placement data, landmark geometry, transparent sorting/index caches,
vehicle LOD/instancing, seat generation/LOD and camera/input code are preserved.
Only the embedded shader compilation glue and seat view-direction varying
change in those specialized renderers. Existing main.cpp development-spawn
edits are preserved. Validation artifacts remain ignored in obj/graphics-g1-check.

Stopped for F5 review; no commit or push.
