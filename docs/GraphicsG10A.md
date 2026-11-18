# G10A — performance and rendering audit (2026-11-18)

Audit only: no application source, shader, geometry, placement or project changes.
Debug x64 build passed. Current Debug objects were linked into an ignored audit
executable using the same Scene, Camera and ColorGrading paths as the application.

## Hardware and method

- GL_VENDOR: **Intel**
- GL_RENDERER: **Intel(R) UHD Graphics**
- GL_VERSION: **3.3.0 - Build 32.0.101.6733**
- Framebuffer: **1280 × 720**; no MSAA.
- Both audit contexts and the actual `Astana3DTour.exe` startup log confirm Intel.
  **NVIDIA RTX 3050 was not tested.** No GPU preferences/system settings changed.
- Normal main.cpp enables depth testing, leaves face culling disabled and requests
  VSync. Audit matched depth/culling but disabled VSync, used a hidden window and
  serialized frames with glFinish. These are uncapped diagnostic results, not F5
  window-title averages. Earlier tests enabling culling are not matched baselines.
- Static views: 12 warm-up frames, 60 measured frames, F6/F7/F8 ON, scene time 8.
  Draw/triangle totals include instances and the final composite; stationary shadow
  maps were reused. Submitted triangles are not visible/rasterized triangles.
- GL call wrappers add some overhead. GPU elapsed queries include pipeline time,
  not an isolated shader profile. Short laptop samples have scheduling/clock noise.

## Representative views

| View | Mean FPS | Mean ms | Draw calls/frame | Submitted triangles/frame |
|---|---:|---:|---:|---:|
| Baiterek / Northern Lights | 50.6 | 19.75 | 3,429 | 2,724,817 |
| Dense central residential | 48.8 | 20.49 | 3,338 | 2,513,837 |
| Botanical Garden lawn | 44.4 | 22.52 | 3,392 | 2,841,517 |
| EXPO / Nur Alem | 48.6 | 20.58 | 3,347 | 2,369,633 |
| Astana Arena exterior | 56.1 | 17.83 | 3,365 | 2,653,059 |
| Astana Arena interior | 49.3 | 20.30 | 3,355 | 2,846,091 |
| Esil / Atyrau Bridge | 62.6 | 15.97 | 3,295 | 2,156,761 |
| Ak Orda | 59.9 | 16.71 | 3,279 | 2,155,413 |

Camera eye → target, in the same order:
`(-8,36,65) → (-60,65,-20)`;
`(320,120,-180) → (230,0,-260)`;
`(-377,3,-141) → (-380,.037,-149)`;
`(-675,24,-5) → (-765,35,-130)`;
`(-280,25,-300) → (-370,15,-435)`;
`(-370,18,-420) → (-400,.6,-435)`;
`(370,8,65) → (510,4,100)`;
`(0,3,285) → (0,25,370)`.

## Matched settings and movement

All eight combinations ran at street eye `(175,5,-254)`, target `(153,9,-238)`.
The initial forward/reverse sweep showed timing drift. One focused OFF/ON/ON/OFF
recheck used 32 frames per block. Other effects stayed ON for individual tests.

| Comparison | OFF FPS / ms | ON FPS / ms | Added wall ms | Added GPU elapsed ms |
|---|---:|---:|---:|---:|
| F6 shadows | 50.88 / 19.65 | 50.23 / 19.91 | 0.26 | 0.78 |
| F7 grading | 51.82 / 19.30 | 49.07 / 20.38 | 1.08 | 0.79 |
| F8 FXAA | 51.99 / 19.23 | 50.21 / 19.92 | 0.68 | 0.88 |
| All three together | 56.58 / 17.68 | 50.18 / 19.93 | 2.25 | 2.25 |

Effects interact through the common composite; individual deltas are not additive.
FXAA had the largest GPU delta in this sample, but all three were close (~0.8 ms).
The larger F7 wall delta includes CPU/scheduling variation; do not treat it as a
precise shader-only ranking. Disabling both F7/F8 bypasses the scene FBO/composite.

Slow lateral movement: 72 frames at 0.12 m/frame, translating eye and target
together. F7/F8 stayed ON. F6 OFF: **49.65 FPS / 20.14 ms**; ON: **46.87 FPS /
21.34 ms**. Shadow projection refreshed **8/72** frames, versus zero refreshes
in every stationary measured view. Refresh frames averaged 21.71 ms and reused
frames 21.29 ms (full-frame measurements, not isolated shadow-pass duration).
Shadow caching is working; moving-view cost also includes view-dependent culling,
LOD and instance-list work. Teleport/view changes refreshed the map before reuse.

## Bottlenecks and visual findings

- Shared legacy Mesh rendering submitted **2,937 draws / 84,390 triangles in every
  tested direction**. Total CPU submission averaged 3.7–4.2 ms in static views.
  Large static landmark/district paths also submit geometry without camera-frustum
  tests. Even the river/palace views submit over 2.15 million triangles.
- GPU elapsed was 11.3–17.8 ms across views. Rendering/overdraw and material shading
  are substantial costs; reducing draw calls alone will not eliminate GPU cost.
- `BotanicalGarden::renderGreenhouseGlass` allocates temporary vectors, sorts and
  re-specifies its index buffer every frame: **one 3,504-byte upload**, including
  stationary and distant views. This differs from retained EXPO/MEGA glass caches.
- No obvious large shadow-acne patches or severe detached contact shadows in the
  sampled images. Local coverage/fading remains the intentional G6 limitation.
  The short motion samples cannot certify all shadow edges or fast flight.
- Thin facade/roof seams, Nur Alem lattice, bridge members, goal nets and distant
  seating retain aliasing/speckling. Some facade edge breakup persists with F6 OFF,
  so it cannot all be attributed to shadow acne. Exact depth-overlap versus
  subpixel-geometry causes need targeted diagnosis, not a global material change.
- No obvious glass ordering reversal in sampled landmark views. A complete moving
  orbit through every glass interior/MEGA was not performed.
- Sky/horizon stayed continuous. Animated water samples at 8/8.25/8.5 s showed no
  abrupt white flashes; this is not a comprehensive temporal-flicker guarantee.
- Garden grass stayed on lawns and off adjacent paving. Elevated views omit it.
  Small facade-camera shifts showed no obvious LOD oscillation; this did not test
  every near/medium/far threshold crossing or prove distant shimmer eliminated.
- Resize 1280×720 → 960×540 → 1280×720 passed. Four FBO completeness checks passed;
  no GL errors were detected during measured frames, resizing or harness cleanup.

## Resources

Steady allocations: **275 buffers / 205.80 MiB**, seven textures / approximately
**29.11 MiB** including mips and shadow depth, one depth RBO / approximately
**3.52 MiB**, two FBOs, 1,247 VAOs and nine programs. Estimated payload total:
**238.43 MiB**, excluding default framebuffer, driver overhead/compression and CPU
copies. DEPTH24 is conservatively budgeted as four bytes; this is not VRAM telemetry.
Largest buffer allocations were 38.84 and 22.97 MiB; architecture dominates the
small vegetation pools. Trees: 292,944 bytes; grass: 1,973,644 bytes;
shadow proxies: 2,114,148 bytes. Shadow depth: one 2048² map (~16 MiB budget).
RGBA16F scene color: ~7.03 MiB; its depth attachment: ~3.52 MiB at 1280×720.

No steady resource-count growth across views/resizes. All tracked buffer, texture,
RBO, FBO, VAO, program and shader names returned to zero after normal harness
destruction in both audit runs. No obvious normal-path GL leak was found; exception
paths and actual resident GPU memory were not exhaustively tested.
The separate hidden production-EXE startup probe confirmed its renderer, but a
window-close request did not terminate that process promptly; only that audit-owned
process was stopped. This probe does not establish an interactive shutdown defect;
normal cleanup claims above apply to the fully completed Scene harness runs.

## Ranked next work — recommendations only

Expected gains below are qualitative, not measured optimization speedups.

| Rank | Recommendation and evidence | Expected FPS benefit | Difficulty | Visual impact | Risk |
|---|---|---|---|---|---|
| 1 | Conservative camera-frustum gates for currently unconditional landmark/district submissions; >2.15M triangles even in remote views | High potential, view-dependent | Medium | None with conservative bounds | Medium: clipped details if bounds wrong |
| 2 | Batch/instance repeated legacy Mesh pieces by material; 2,937 small draws every view | High CPU-side potential; GPU gain may be smaller | Medium–high | Preserve existing models | Medium: transforms/material/order |
| 3 | Audit closed opaque meshes for selective back-face culling and suitable distant LOD; main currently leaves culling OFF | Medium GPU potential, unmeasured | Medium–high | Preserve silhouettes; keep open/two-sided geometry | Medium–high: winding/interiors |
| 4 | Profile common composite sampling/color conversion and shadow-receiver sampling before tuning; ~0.8 ms GPU delta per effect | Low–medium, bounded by measured effect cost | Medium | Retain approved AA/color/shadow quality | Medium |
| 5 | Cache greenhouse glass order/uploads when unchanged and cull the conservatory when irrelevant | Small overall FPS gain; straightforward waste removal | Low–medium | None if sorting invalidation is correct | Low–medium |

Separately prioritize targeted thin-geometry/depth-order diagnostics for the visible
landmark seam and seating aliasing. Do not apply generic residential filtering or
blanket culling to all landmarks. No proposed optimization was implemented.

## Reproduction and artifacts

Ignored `obj/graphics-g10a-audit/` contains `audit.cpp/.cmd`,
`effectcheck.cpp/.cmd`, `results.log`, `effects.log`, `measurements.json`, startup
logs and matched PNG captures. From the repository root, compile with
`cmd /c obj\graphics-g10a-audit\audit.cmd`, then run the resulting audit executable.
The scripts use installed VS2022 and the current Debug object files except main.obj.
Only this documentation is a new project deliverable. No commit/push. G10B not started.
