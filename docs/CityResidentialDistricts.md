# City Development Part 3B - Five residential neighborhoods

All five remaining documented parcels are populated. The completed A3 premium
complex, its generator and its courtyard are unchanged. No other district,
road, shader, camera control or existing optimization is modified.

## Placement

Coordinates below are world X/Z metres; footprints are X by Z. Ground-floor
datum is Y=0.18, lobby height 4.20 m and residential floor spacing 3.35 m.
Heights include the near-detail crown cap. All apartments are inside their
assigned plot, including entrance canopies and 2.5 m pedestrian aprons.

| Plot | Bounds X / Z | Size | Buildings |
| --- | --- | --- | --- |
| A1 | -659..-496 / -713..-610 | 163 x 103 m | 3 |
| A2 | -464..-301 / -713..-610 | 163 x 103 m | 3 |
| A4 | -464..-301 / -859..-747 | 163 x 112 m | 3 |
| G1 | -335..-237 / 43..123 | 98 x 80 m | 2 |
| G2 | -404..-296 / 165..269 | 108 x 104 m | 2 |

| Building | Center X/Z | Footprint | Floors | Height | Style |
| --- | --- | --- | ---: | ---: | --- |
| A1-1 | -632 / -665 | 24 x 46 | 12 | 42.12 m | A |
| A1-2 | -523 / -657 | 28 x 38 | 16 | 55.52 m | C |
| A1-3 | -577 / -692 | 42 x 22 | 10 | 35.42 m | A |
| A2-1 | -439 / -661 | 26 x 42 | 14 | 48.82 m | C |
| A2-2 | -327 / -656 | 26 x 38 | 12 | 42.12 m | A |
| A2-3 | -383 / -692 | 44 x 22 | 16 | 55.52 m | C |
| A4-1 | -438 / -795 | 28 x 36 | 22 | 75.97 m | B |
| A4-2 | -328 / -782 | 28 x 32 | 18 | 62.22 m | C |
| A4-3 | -379 / -837 | 42 x 26 | 26 | 89.37 m | B |
| G1-1 | -317 / 82 | 22 x 48 | 10 | 35.42 m | A |
| G1-2 | -264 / 96 | 28 x 28 | 12 | 42.12 m | C |
| G2-1 | -330 / 190 | 26 x 30 | 12 | 42.12 m | A |
| G2-2 | -315 / 244 | 24 x 30 | 16 | 55.52 m | C |

Style A uses ivory/beige stone, dark frames and selective recessed loggias.
Style B uses warm brick, pale stone piers and a stronger stepped crown.
Style C alternates gray/ivory facade sections with corner frames and selected
glass-like loggia guards. All three use actual wall apertures, blue-gray
opaque reflective windows, floor bands, recessed door groups and metal
entrance canopies. The upper four floors step back twice; parapets and small
technical roof housings finish the silhouettes. There are five A buildings,
two B towers and six C buildings, rather than thirteen identical copies.

Building bodies have at least 14 m diagonal/orthogonal separation. The G2
view axis X=-390..-355 remains free of building mass and tree crowns. Its
two buildings occupy the eastern portion, leaving a generous western lawn.
The immediate colonnade reservation X=-400..-340, Z=23..140 stays untouched.

## Courtyards and access

Five linked courtyards provide light stone paths, lawn quadrants and lobby
branches, with **20 trees, 40 shrubs and 20 wood/metal benches** in total.
Four-metre main paths stay open. Compact G1 places its benches to one side
of the circulation strip. Paving is unioned before upload and gently ramps
to entrance thresholds; individual tiles are not separate objects.

Ten short verge connections meet retained kerbs or existing Garden paving.
Their last 2 m rise from Y=.06 to .14. They do not enter asphalt or introduce
new road crossings. Center X and south/north connection endpoints Z:

| Plot | Path center X | South end Z | North end Z |
| --- | ---: | ---: | ---: |
| A1 | -578 | -725.3 | -589.2 |
| A2 | -383 | -725.3 | -589.2 |
| A4 | -382.5 | -874.8 | -734.7 |
| G1 | -292 | 23 | 140.3 |
| G2 | -371.5 | 149.7 | 280.8 |

Future street sidewalks and the previously reserved crossings remain future
road work. There are no new parking lots, interiors or playgrounds.

## Rendering and validation

A single reusable generator uses the unchanged PremiumResidenceGeometry
indexed primitives and facade frames. Shared material batches, one VAO/VBO/EBO
and five parcel ranges replace per-window or per-building draw calls. Each
parcel has three immutable building LOD meshes and one shared site mesh.
Footprints vary, so facade elements are baked into static batches rather
than introducing a second instancing renderer. Initialization uploads once.

| All five visible | Building triangles | Site triangles | Total | Draws |
| --- | ---: | ---: | ---: | ---: |
| LOD0 near | 211,326 | 9,950 | 221,276 | 61 |
| LOD1 medium | 123,524 | 9,950 | 133,474 | 61 |
| LOD2 far | 29,518 | 9,950 | 39,468 | 56 |

LOD1 simplifies reveals, loggia hardware and mullions. LOD2 drops their
relief while retaining every window, floor course, setback and entrance.
Far building geometry is 86% lighter than near geometry. Near-to-medium
switches at 165 m / returns at 120 m; medium-to-far switches at 420 m /
returns at 350 m, measured to the nearest building AABB. Initial selection
uses 145/420 m. Five conservative group AABBs include all roofs and verge
links and reject groups outside the camera frustum. Hysteresis, culling and
drawing use fixed arrays with no per-frame allocation or buffer upload.
Static GPU storage across all LODs and sites is 28,620,904 bytes.

Debug x64 build passed; no new C++ warnings. Native initialization checks
finite positions/UVs/normals, unit normals, valid indices and nondegenerate
outward winding in all LODs. Footprint, separation and G2 view reservations
are checked against the shared parcel plan. Uploaded geometry checks found
zero overlap between new paving and existing road asphalt, and no duplicate
paving surfaces. Every unrelated source retains its starting SHA256,
including A3, EXPO, all road modules, shaders and optimized renderers.

A full-scene pass inspected all five plots from above, southern neighborhoods,
a courtyard entrance, close facade and G2 pedestrian view. All three LODs
and full frustum rejection were observed. Shader compilation, OpenGL errors,
depth/blend/material flags and polygon-offset restoration passed.

One representative stationary sample: **86.43 FPS / 11.57 ms**, 120 frames,
Debug x64, hidden 1280 x 720 Intel UHD Graphics context, eye
(-770,155,-540), target (-460,25,-734), GPU-synchronized and vsync off.
A1/A2/A4 used LOD1; G1/G2 were culled. New geometry submitted in that view
was 100,906 triangles / 37 draws. This is an actual offscreen sample, not a
visible F5 result or a before/after performance comparison.

Changed: CityResidentialDistricts.cpp/.h, Scene.cpp/.h, the two Visual Studio
project files, ResidentialDistrictPlan.md and this document. Temporary
validation artifacts remain in ignored obj/city-residential-part3b-check.
Stop for F5 visual/performance review. No commit or push.
