# Graphics G6C — landmark and vegetation casters

The existing SunShadows depth program, **one 2048² map**, approximately **300×347 m ground coverage**, G6B filtering/bias/fading, caching and F6 control remain unchanged. No new shadow shader, reflection pass or cascades were introduced.

## Connected geometry

- **Vegetation:** Botanical Garden's existing rounded, slender and conifer planting records, lakeside trees and perimeter trees; individual Scene/Abu Dhabi landscaping trees; background residential courtyard trees and inexpensive shared shrub meshes; Astana Arena's existing batched trunks/crowns. Tree positions are read from the existing placement data, never recreated from guessed coordinates.
- Garden crowns use low-poly two-lobe, slender ellipsoid or tiered-cone silhouettes. Individual Tree objects reuse their actual trunk and crown transforms. Position-only proxies are generated/uploaded once and grouped into 96 m cells, then culled against the light frustum. There are no per-tree draw calls, detailed leaves or moving foliage. Static batching avoids another instancing shader path; residential building instancing/LOD remains intact.
- **Baiterek:** existing opaque structural transforms and a coarse gilded sphere, batched into a single landmark group; shaft glazing omitted.
- **EXPO/Nur Alem:** coarse opaque museum sphere and podium silhouettes. The museum facade already renders opaque with alpha 1; it is distinct from EXPO's actual blended glass canopy, which is excluded. Other EXPO pavilion/canopy structures are outside this extension.
- **Khan Shatyr:** existing opaque membrane, mast, rim, plinth/wall and roof-transition ranges; detailed cables and entrance/storefront glass omitted.
- **Astana Arena:** existing opaque exterior/roof ranges with the real retractable opening, plus existing planting batches. Seating, pitch, parking surfaces and entrance glazing omitted.
- **Talan Towers:** existing opaque reflective tower shells and roofs, podium and canopy structure. Gallery/entrance glass omitted.
- **Abu Dhabi Plaza:** compact box silhouettes derived from the existing opaque tower-massing and structural transforms; lobby/door panes omitted. These tower facades render opaque, not through the blended glass system.
- **Ak Orda:** batched existing opaque structural transforms plus existing curved facade, portico, dome and spire ranges; window ranges omitted.

New silhouettes belong only to the depth pass. All original visible landmark geometry and rendering functions remain. Specialized glass, water, stadium and other bespoke receiver paths remain as in G6B; this does not make every material or every city planting module a shadow receiver/caster.

## Cost and verification

The shared position-only proxy buffer contains **51,476 shadow-only triangles**, **2,578 primitive components** in **21 spatial/landmark groups**, using **1,514,748 bytes** of vertex/index storage (~1.45 MiB). This is not a count of trees/buildings: components include structural beams and crown lobes. No visible triangles or color-pass draw calls were added. Other caster ranges reuse existing GPU buffers.

In the matched Garden overview, a depth update changed from **5 draws / 29,586 triangles** to **27 draws / 125,759 triangles**. Cached frames add no depth draws. Region-dependent total depth work in the inspected views ranged from **17–30 draws / 38,132–133,127 triangles**. All caster culling occurs only when the existing map needs updating.

Debug x64 build passed. Full-scene shader, framebuffer, F6 press/hold/focus, disabled-pass, stationary-cache and OpenGL state/error checks passed on Intel UHD. Matched Botanical Garden path/overview, Baiterek, EXPO, tree-lined district street and Arena views were inspected. Tree silhouettes meet the ground/paving; landmark shadows align with the existing sunlight. All six OFF captures are byte-identical to G6B. G6B's shadow implementation and G1–G5 shader/material files retain their baseline hashes.

One brief matched moving Garden overview at Debug 1280×720, 45 measured frames after warm-up: **OFF 67.79 FPS / 14.752 ms; ON 66.12 FPS / 15.125 ms**. ON updated the map on 17 frames; OFF performed zero updates. This short hidden-window sample is not a guaranteed visible F5 frame rate.

Modified: `ShadowProxies.cpp/.h`; shadow-only hooks in `Scene`, `Tree`, `BotanicalGarden`, `BackgroundResidential`, `AbuDhabiLandscaping`, `Baiterek`, `ExpoDistrict`, `NurAlem`, `KhanShatyr`, `AstanaArena`, `TalanTowers`, `AbuDhabiPlaza`, `AkOrda` (each `.cpp/.h`); both Visual Studio project files; README and this document.

Limits: local map coverage, simplified solid crown/structural silhouettes, no leaf translucency, and incomplete vegetation coverage outside the listed modules (including MEGA/riverfront planting). True blended canopy glass, vehicles and spectators do not cast shadows. Stopped for F5 review; no commit or push.
