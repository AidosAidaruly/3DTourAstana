# Graphics G8B - Distant facade filtering and LOD refinement

Targeted the shared instanced renderer used by **287 background buildings**. Its medium LOD retained 5 cm center mullions, and medium/far facades retained individual panes and 16 cm floor bands. Pane/backing offsets of only 8-18 mm also caused depth fighting at city distances. Dedicated premium residences, EXPO architecture and landmark paths were inspected and left unchanged.

- Medium/far windows reduce contrast toward their actual adjacent ivory/stone/brick palette as their estimated projected size falls below six pixels. Thin surrounds and floor bands fade according to their own projected widths. Derivatives use stable local coordinates, handling instance scale, rotation, oblique views and framebuffer resolution. A residual window rhythm remains; facades do not become blank.
- Baked pane-to-pane tint and coating gradients fade gently from 160 to 520 m. The existing G5 glass reflection/light function is preserved. Wall palette metadata uses the existing material scalar, without changing the vertex stride, instance buffer or adding textures.
- Simplified overlays receive approximately 1-5 D24 depth steps of clip-space ordering, preventing panes, backing plates and floor bands from alternating triangles. World-space vertices/footprints and near meshes are unchanged.
- Removed subpixel center mullions from medium meshes. Far transition is **280 m outward / 230 m inward**, formerly 300/245 m. Near initialization and 130/90 m transition hysteresis remain unchanged. Group batching, cached culling and static uploads remain intact.

Shared mesh triangles near/medium/far: **77,098 / 16,694 / 11,446**, previously **77,098 / 21,450 / 11,446**. Medium geometry decreases by 4,756 triangles (22.2%); allocated renderer buffers decrease by 437,552 bytes. No added draws, buffers, rendering passes or shadow resources. Existing glass exclusion in the shadow pass remains compatible with the metadata tags.

Debug x64, mesh index/winding validation, full-scene shaders/OpenGL, all F6/F7/F8 combinations and stationary shadow reuse passed. Matched dense-city, EXPO-distance and close-up views were inspected, plus 36 frames of slow translation. A same-context, 12-frame slow-yaw test reprojected previous images to remove expected camera motion: facade luminance residual averaged **10.12 -> 4.81 / 255**. This is a local diagnostic, not a general temporal-AA guarantee. The close facade comparison was byte-identical; 4 cm oscillations around the far boundary did not cause LOD cycling.

Brief matched full-scene Debug 1280x720 sample, Intel UHD, F6/F7/F8 ON: **76.9 -> 77.8 FPS** (13.00 -> 12.85 ms). No measurable regression; the small difference is within short-sample variability, not a claimed speedup.

Changed only `src/BackgroundResidential.cpp` and this document. Sky, daylight, tone mapping, FXAA, water, controls, placement and unrelated work are preserved. Remaining limitations: distant silhouettes and unmodified dedicated facade systems can still alias; no temporal history or general shimmer elimination was introduced. Stopped for F5 review; no commit or push.
