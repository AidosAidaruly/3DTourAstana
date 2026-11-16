# City Development Part 2 - Secondary streets and residential plot plan

Planning coordinates are X/Z in scene metres. No residential buildings or
visible parcel markers are included in this stage. This plan was selected
before geometry generation from CityRoadNetwork/CityRoadPlan, district roads,
NorthernLightsSite, Arena parking, Abu Dhabi vegetation/furniture and the
existing terrain limits. Part 1's routes and landmark placements are retained.

## Selected streets

- S1 west collector: (-675,-589) to (-675,-860), 10 m wide.
- S2 southern link: (-675,-860), R20 bends via (-655,-880) and
  (-305,-880), ending (-285,-860), 10 m wide.
- S3 east collector: (-285,-860) to (-285,-589), 10 m wide.
- S4 west cross-street: (-670,-730) to (-480,-730), 9 m wide.
- S5 east cross-street: (-480,-730) to (-290,-730), 9 m wide.
- S6 northern central street: (-480,-589) to (-480,-730), 9 m wide.
- S7 southern central street: (-480,-730) to (-480,-875), 9 m wide.
- N1 Garden north connector: (-460,18), north to (-460,125), R20
  bend to (-440,145), east to (-260,145), then a smooth cubic to
  (-209,122) and a flared mouth at existing street edge (-205,122), 9 m.
- N2 Garden residential loop: (-420,149.5), north to (-420,270),
  R15 bend to (-405,285), east to (-295,285), R15 bend to (-280,270),
  south to (-280,149.5), 8 m wide.

The three southern entries meet Part 1's south edge Z=-589. Local kerb,
edge-marking and median openings permit turning without moving that arterial.
Internal street junctions have 4 m kerb returns; curved perimeter links use
15-20 m centreline radii. N1 links the Garden road to the Abu Dhabi/Northern
Lights central street network. N2 is connected at both ends to N1.

## Future development envelopes (not building footprints)

| Plot | X bounds | Z bounds | Size | Suggested future use |
| --- | --- | --- | --- | --- |
| A1 | -659..-496 | -713..-610 | 163 x 103 m | Premium 8-16 floor courtyard apartments |
| A2 | -464..-301 | -713..-610 | 163 x 103 m | Mixed residential/commercial, 8-16 floors |
| A3 | -659..-496 | -859..-747 | 163 x 112 m | Allocated in Part 3A: 16/22/28-floor premium complex |
| A4 | -464..-301 | -859..-747 | 163 x 112 m | Mixed-height residences, 12-28 floors |
| G1 | -335..-237 | 43..123 | 98 x 80 m | Premium 8-12 floor urban apartments |
| G2 | -404..-296 | 165..269 | 108 x 104 m | Premium 8-16 floor courtyard development |

Envelopes reserve room for later courtyard, entry, parking and planting design;
maximum envelopes are not permission to fill them with building mass. Future
models should use the design quality of EXPO Residence/Premium Residence,
not crude placeholder boxes. Leave setbacks and generous spacing between
future towers. The immediate colonnade sightline strip X=-400..-340, Z=23..140 is not a
building plot. Inside G2, keep X=-390..-355, Z=165..269 as an open courtyard
view axis; the recorded envelope includes that required open space, which
must not become future building mass. Roads do not obstruct the axis vertically.

## Clearances and reservations

Southern streets lie below the Arena and east of MEGA (whose podium reaches
X=-866); no southern street reaches the river or another landmark. N1/N2 lie
east of the premium EXPO residential plot ending at X=-514 and north of the
Garden's outside walk ending at Z=23. N1's local driveway opening at X=-460
crosses only the existing outer walk/kerb, away from the colonnade and crossing.
The central street junction at (-205,122) fits between existing Abu Dhabi
trees at Z=112/132 and streetlights at Z=116/140; these objects must not move.
The six-space parking court at Z=157.5..172.5 is outside N1's route.

Reserve later pedestrian crossings near the three southern arterial mouths,
the internal cross-street junctions, and the Garden loop entries. Keep 4-6 m
of roadside land for later sidewalks/green strips. These are documented
reservations, not new crossing objects or raised terrain patches. No terrain
extension, cars, buildings, trees, lights or parking lots are added.

Khan's west strip between the existing loop and Part 1 arterial is too narrow
for the target residential plots. Northern Lights' immediate land contains
approved parking and public space. Do not force additional streets there.
Validation results and rendering cost will be appended after implementation.


## Implemented result

Nine named connected secondary streets add **2,398.28 m** of road. All typical
streets have one lane per direction. The southern perimeter uses 10 m bodies;
internal streets and the Garden connector use 9 m; the Garden loop uses 8 m.
Local R4 mouths flare wider than the street body to provide turning space.
S1/S2/S3 form one continuous rounded southern perimeter, not three dead ends.
S4/S5 and S6/S7 meet at a single rounded four-way crossroads.

| Street | New centerline length | Typical width |
| --- | ---: | ---: |
| S1 | 271.00 m | 10 m |
| S2 | 412.82 m | 10 m |
| S3 | 271.00 m | 10 m |
| S4 | 190.00 m | 9 m |
| S5 | 190.00 m | 9 m |
| S6 | 141.00 m | 9 m |
| S7 | 145.00 m | 9 m |
| N1 | 379.34 m | 9 m |
| N2 | 398.12 m | 8 m |

The six plot envelopes listed above total approximately 89,162 square metres.
They are planning data only: no parcel slabs, fences, buildings, placeholders
or visible plot overlays are generated. Their setbacks preserve room for
later pedestrian routes, green strips and individual complex planning.

### Junctions and pedestrian reservations

Ten three-arm/T-junctions and one four-way crossroads are added:

- Southern arterial street centers: (-675,-580), (-480,-580), (-285,-580).
- Perimeter/internal T-junctions: (-675,-730), (-285,-730), (-480,-880).
- Central crossroads: (-480,-730).
- Garden/central road junctions: (-460,12), (-200,122), at the existing
  carriageway centers; new street edges meet Z=18 and X=-205 respectively.
- Garden loop entries: (-420,145), (-280,145).

The three Part 1 arterial entries have precise local kerb/edge-marking cuts
and median gaps with tapered noses. All original arterial centerlines,
widths, elevations and bends remain fixed. Old road surfaces are not covered
with another asphalt layer. Median footprints locally become level asphalt
where turning is required. Other Part 1 geometry is unchanged outside
approximately 30 m either side of each new entry.

Nine future crossing reservations are stored in SecondaryStreetPlan.h.
Each reserves a 4 m-wide crossing and space at both ends for future sidewalks:

| Reservation | X/Z center | Walking direction across street |
| --- | --- | --- |
| CP1 | -675 / -606 | East-west |
| CP2 | -480 / -606 | East-west |
| CP3 | -285 / -606 | East-west |
| CP4 | -500 / -730 | North-south |
| CP5 | -480 / -750 | East-west |
| GP1 | -460 / 35 | East-west |
| GP2 | -420 / 165 | East-west |
| GP3 | -280 / 165 | East-west |
| GP4 | -370 / 145 | North-south, on the colonnade/courtyard view axis |

Crossings, signals and detailed sidewalks are reserved for later work.
The N1 connection opens only local Garden outside paving and a small piece
of Abu Dhabi's west sidewalk/kerb at Z=110..134. Exact polygon subtraction
retains all residual paving, including support for the existing light at
(-207,116). The two closest roadside tree crowns at (-216,112/132) stay
clear. Existing side parking, planting, lights and benches are unchanged.

### Mesh and rendering impact

- CityRoadNetwork: **29,964 triangles, four opaque indexed draws**;
  **+15,611 triangles**, no new material batch or draw call.
- Garden paving/kerb/marking changes: **+402 triangles**, unchanged draws.
- Scene: two extra sidewalk/kerb rectangle pieces balance two removed
  junction-conflicting marking cubes: zero net triangles or draws.
- **Net scene impact: +16,013 triangles, zero added draw calls.**
- Network vertex/index storage: 2,372,176 bytes, +1,278,948 bytes.

All secondary asphalt, junction infills, kerbs and markings are built once
at initialization into the existing road material batches. Overlapping
junction surfaces are removed before upload. Boundary kerbs are trimmed at
crossroads rather than carried across driving lanes. Initialization uses
reusable route/parcel data; no per-frame generation, allocations, sorting or
buffer updates are introduced. The existing road render function and shaders
are unchanged. Glass caches, parked vehicles, stadium seats, camera controls
and FPS instrumentation are untouched.

### Verification and limitations

Debug x64 builds successfully, with no new C++ warnings. Preexisting GLFW
missing-PDB linker warnings remain. Finite vertex/UV data, valid indices,
unit normals and nondegenerate triangle winding pass. Uploaded-geometry
checks verify nine connected routes, six disjoint legal parcels, clear
retained lamps/crowns and no substantive asphalt self-overlap or kerb/paving
obstructions. Maximum numerical asphalt seam overlap is approximately
0.0000665 square metres; kerb/asphalt seam overlap is below 0.000001 square
metres. These are float-precision seams, not duplicate paving layers.

Garden asphalt and all non-road Garden geometry are identical to Part 1.
Part 1 road mesh changes are confined to the three arterial junction areas.
Its shared alignment header is byte-identical. Every unrelated source file,
landmark, shader, camera and optimized renderer retains its starting hash.
No terrain extension is required; all added streets fit the existing ground.

One full-scene validation pass inspected a high aerial view, the southern
crossroads, N1's Abu Dhabi connection and a southern arterial entry. Shader
compilation, OpenGL errors and depth/blend/material-state restoration passed
in a hidden 1280 x 720 Intel UHD Graphics context. No FPS benchmark or claim
of maintaining a measured framerate is made in this stage.

No unresolved street ends remain in these two neighborhoods. Development
around Khan and the immediate Northern Lights public realm is deferred
because the remaining strips are narrow or already occupied. The existing
Esil/Talan vehicle-link constraint remains outside this stage. Further city
expansion is not generated automatically.

Changed: src/SecondaryStreetPlan.h, src/SecondaryStreets.cpp/.h,
src/CityRoadNetwork.cpp, src/BotanicalGarden.cpp, src/Scene.cpp,
Astana3DTour.vcxproj/.filters and this document. All unrelated work is
preserved. Stop for F5 visual review. No commit or push.


## Allocation update - City Development Part 3A

A3 is now occupied by ONE completed three-building premium residential
complex. Do not allocate this parcel again. At completion of Part 3A,
A1, A2, A4, G1 and G2 remained undeveloped. The earlier floor ranges were indicative planning suggestions;
the new user-requested composition is 16, 22 and 28 floors.

See [ResidentialBlockA3.md](ResidentialBlockA3.md) for the exact placement,
architectural details, courtyard, street-edge connections, LOD and validation.
No street alignment, parcel boundary or existing district geometry changes.

## Allocation update - City Development Part 3B

All five remaining plots are now occupied: A1/A2/A4 each have three
medium-detail residences; G1/G2 each have two. There are 13 new buildings,
five shared courtyards and ten short walking links to retained road edges
or Garden paving. A3 and the existing EXPO residences are unchanged.
The G2 view axis remains open. No planned residential plot remains vacant.

See [CityResidentialDistricts.md](CityResidentialDistricts.md) for the exact
building coordinates, footprints, floor counts, heights, styles, access
connections, LOD/culling costs and measured rendering sample. These
allocations supersede the future-use suggestions above; parcel boundaries
and all approved street alignments remain fixed.
