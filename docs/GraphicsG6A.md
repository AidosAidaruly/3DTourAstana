# Graphics G6A — initial local sunlight shadows

## Scope and controls

**F6** toggles shadows, initially ON. The existing FPS/movement title also shows ON/OFF. Press/hold/release is edge-triggered; unfocused input cannot toggle shadows. Disabling skips the depth pass entirely.

One OpenGL 3.3 depth-only framebuffer has a **2048×2048 D24** texture, with no color attachments. Settings are centralized in `SunShadows::Settings`: 150 m orthographic half extent, 1–600 m light-depth range, 0.025 m minimum plus 0.12 m slope-dependent receiver bias, and a 10% edge fade. The 300×300 m light-space footprint covers approximately 300×347 m on level ground, rotated with the existing G2 sun direction `(-0.5,-1,-0.3)`. The center follows the camera with at most 100 m forward ground look-ahead.

Light-space texel snapping stabilizes the map. Because the selected geometry and sun are static, an unchanged fitted projection reuses its depth texture. Moving outside the snapped region updates it; no mesh or GPU-buffer rebuilding occurs. Four bilinear comparison taps provide PCF (16 filtered depth comparisons). Small polygon offset and slope-aware receiver bias limit acne without front-face culling or a large normal offset.

## Casters and receivers

- Casters: nearby `BackgroundResidential` buildings, using their existing LOD 2 meshes, instancing and material attributes. Groups are culled against the light frustum independently of the cached camera culling/LOD state. Both residential glass tags are discarded in the depth shader.
- Northern Lights contributes its existing opaque silver/metal structure when its conservative bounds intersect the local map. Tower/podium glass and site-material batches are excluded.
- Receivers: main terrain, standard roads, generic paving and compatible opaque main-shader surfaces; background residential masonry, stone, roofs, metal and site paving. Only the direct sunlight term and compatible metal sun highlights are attenuated; ambient fill remains.
- Specialized water, bridge, stadium, glazing and other bespoke material responses retain their G1–G5 shading. There are no new transparent casters, shadow sorting, cubemaps or cascades. Other landmarks, older residence renderers, vegetation, vehicles and stadium seats are not yet shadow casters.

## Verification and cost

Debug x64 build passed. Shader compilation and framebuffer completeness passed on Intel UHD. Matching ON/OFF street, facade, elevated and Northern Lights views were inspected. The shadows visibly follow the sun and meet the ground, without obvious acne/detachment in these captures. F6's press/hold/release/focus logic, disabled-pass skipping and stationary-map reuse passed. Framebuffer, viewport, depth/blend/offset state and texture-unit restoration passed; no OpenGL errors occurred.

One short matched **moving** street path at Debug 1280×720, same camera path/GPU, 45 measured frames after warm-up: **OFF 90.52 FPS / 11.047 ms; ON 90.17 FPS / 11.091 ms**. All 45 measured ON frames updated the map; OFF performed zero updates. The sample is subject to timing variation and does not establish a guaranteed frame rate.

No geometry or color-pass draw calls were added. Tested central updates submitted **40–42 buildings, 18–21 depth draws and 39,696–40,300 depth triangles**. The Northern Lights view submitted 10 architectural objects in 5 depth draws (29,586 triangles). Cached stationary frames add no depth draws. There is one persistent depth texture/framebuffer/program, not per-object allocations.

Source checks retain all existing geometry generation, placements, vertex colors/layouts, G1/G2 libraries and G3–G5 material code; receiver changes only modulate direct light. Single-map coverage fades out locally, and shadow silhouettes use existing low-detail meshes, so small facade details and distant buildings do not have full shadow coverage. No claim of city-wide shadows is made.

Stopped for F5 visual review. No commit or push.
