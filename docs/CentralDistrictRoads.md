# City Development Part 6A - Baiterek / Esil central district

Roads and parcel planning only. No buildings, visible parcel markers, raised
plot surfaces or new landscaping are generated. Coordinates are world X/Z
metres. Scene geometry was inspected before choosing these routes.

## Site and existing-network connection

The developed portion occupies approximately X=150..310, Z=-425..-15, east
of the existing central buildings and west of the Esil waterfront. There
is no vehicle street directly beside Baiterek on its positive-X side.
Crossing its pedestrian axis to reach a western street would violate the
task. Instead, a clear gap joins Khan Shatyr's eastern R132..140 outer road
at (140,-420). Its existing western feeder already joins the city boulevard.

Only four eastern outer-kerb segments, between +/-5.625 degrees, are opened.
Existing circular asphalt, inner kerbs, parking, trees and other Khan
geometry remain unchanged. Cubic mouth curves follow the actual polygonal
ring tangent and share its edge chords. The mouth tapers from about 27.4 m
at the ring to a 10 m street. Kerb height blends from Y=.11 to Y=.14.
The nearest outer lamps and all tree crowns remain clear.

## Implemented streets

All ordinary carriageways are **10 m wide**, with dashed centerlines and
two traffic directions. Asphalt is Y=.04, markings Y=.047, and 0.20 m-wide
kerbs rise to Y=.14. No terrain or river geometry changes.

| Street | Centerline and bends | Length |
| --- | --- | ---: |
| C1 Khan east feeder | (140,-420) to (280,-420), R20 turn centered (280,-400), ending (300,-400) | 171.41 m |
| C2 west local street | (195,-420), (195,-345), S bend to (155,-265), straight to (155,-105), S bend to (180,-45), ending (180,-40) | 399.14 m |
| C3 river-buffer street | (300,-400) to (300,-40), R20 turn centered (280,-40), ending (280,-20) | 391.41 m |
| C4 south cross street | (155,-240) to (300,-240) | 145.00 m |
| C5 north cross street | (155,-130) to (300,-130) | 145.00 m |
| C6 north return | (280,-20) to (200,-20), R20 turn centered (200,-40), ending (180,-40) | 111.41 m |

Total new length: **1,363.37 m**. Six T junctions comprise the existing ring
mouth and (195,-420), (155,-240), (300,-240), (155,-130), (300,-130).
Internal corner returns use R6. Three R20 perimeter bends and two smooth
S bends avoid abrupt square turns. No four-way junction is needed.

Asphalt and corner infills are unioned once before upload. Internal kerbs
are removed at junctions; markings stop near turning areas. Reserve about
**5 m beside each street** for future sidewalks and planting. No finished
sidewalks, crossings or traffic furniture are added now.

## Future parcels (not rendered)

Four buildable parcels have public street frontage. A 16 m pedestrian/green
strip, X=215..231, Z=-227..-143, divides the middle block into two lots.

| Parcel | X bounds | Z bounds | Dimensions | Recommended use |
| --- | --- | --- | --- | --- |
| C-P1 | 212..287 | -402..-253 | 75 x 149 m | Residential, 6-12 floors, shared courtyard |
| C-P2 | 169..215 | -227..-143 | 46 x 84 m | Office, 4-8 floors, retail base |
| C-P3 | 231..287 | -227..-143 | 56 x 84 m | Commercial, 2-5 floors, pocket garden |
| C-P4 | 196..287 | -117..-33 | 91 x 84 m | Mixed-use / residential, 6-10 floors |
| C-G1 | 165..315 | 0..125 | 150 x 125 m | Protected green public space / bridge view; no buildings |

Building envelopes total 27,387 square metres. Put taller portions toward
the west and keep river-facing edges low. These are future design limits,
not instructions to fill whole envelopes with building mass.

## Protected space

- Baiterek plaza and the north/south pedestrian axis remain unchanged.
- The northern return's outer kerb stops at about Z=-14.8. C-G1 keeps the
  diagonal Baiterek-to-bridge view and Z=100 approach free of vehicle traffic.
- Ak Orda's grounds and approaches remain intact; no road is forced across
  the ceremonial gardens to make a northern connection.
- X=315..330 is reserved as an additional green setback before the existing
  waterfront public space. Road/kerb geometry reaches only X=305.2: at least
  24.8 m from the protected X=330 envelope and a sampled 65.8 m from the
  actual upper bank. Future sidewalk reservation still fits this buffer.
- Talan's forecourt, both existing eastern buildings, all residential plots,
  parking, riverside paths, slopes and Atyrau Bridge remain unchanged.

## Rendering and verification

CentralDistrictStreets emits during initialization into CityRoadNetwork's
existing asphalt, stone and paint batches. Grass and rendering code are
unchanged. No new transparency, dynamic upload or per-frame generation.

- CityRoadNetwork: 29,964 -> **39,885 triangles**, still **four draws**.
- Added road geometry: **9,921 triangles**: 2,555 asphalt, 6,900 kerb,
  466 marking triangles.
- Local Khan curb opening: **16 triangles removed**, unchanged material draw.
- Net scene impact: **+9,905 triangles / zero extra draw calls**.
- City road vertex/index storage: 2,372,176 -> 3,207,932 bytes.

Debug x64 build and full-scene OpenGL checks pass. All new vertices, UVs and
normals are finite; unit normals, valid indices and nondegenerate outward
winding pass. All six routes form one connected network. An actual-asphalt
audit of 1,547 centerline samples found no gaps. Protected geometry and
parcel clearances were checked against both the plan and uploaded meshes.

No substantive duplicate asphalt layers or lane-blocking kerbs were found.
Maximum float seam overlap: 0.000113 square metres between new asphalt
triangles, 0.000000021 at the retained ring and 0.00000131 at curb/asphalt
edges. Every old city material's vertex/index prefix is byte-identical.
All Khan geometry outside the four outer-kerb cuts is byte-identical.
Every unrelated source retains its initial hash.

Inspected an elevated Baiterek-to-Esil overview, a local T junction, the
preserved pedestrian axis and the new Khan connection. Shader compilation,
OpenGL errors, depth/blend/material flags and polygon-offset restoration
passed in a hidden 1280 x 720 Intel UHD Graphics context. No FPS benchmark
or visible F5 performance claim is made.

Added: CentralDistrictPlan.h, CentralDistrictStreets.cpp/.h, this document.
Modified: CityRoadNetwork.cpp, KhanDistrict.cpp (local curb opening),
Astana3DTour.vcxproj/.filters, CityRoadNetwork.md (documentation link).
Scene, shared shaders, camera, all 100 background placements, vehicle LOD,
stadium seats and glass caches are unchanged. Verification artifacts are
ignored under obj/central-district-roads-check.

Stopped for F5 review. No buildings, commit or push.
