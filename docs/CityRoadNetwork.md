# Main City Road Network - Part 1 plan

Subsequent work: [Central District Roads - Part 6A](CentralDistrictRoads.md)
adds six eastern streets, four future urban parcels and a protected green
view corridor. The Part 1 geometry documented below is unchanged.

All coordinates are procedural-world X/Z metres, not real-world geography.
The plan was selected before road mesh generation after inspecting Scene,
BotanicalGarden, ExpoUrbanRoads, MegaSilkWaySite, AstanaArenaParking,
NorthernLightsSite and the current KhanDistrict/vegetation/furniture modules.

## Three selected corridors

1. Southern district boulevard: EXPO frontage edge (-735,-416), then
   (-735,-550), 30 m-radius turn to (-705,-580), east to (-238,-580),
   30 m-radius turn to (-208,-550), north to (-208,-260), another 30 m
   turn to (-238,-230), flared T-mouth at Garden road edge (-244,-230).
   Main overall width 18 m, including a 2 m median on suitable sections;
   taper to 12 m before the Garden approach. Existing street centers at
   the terminals are (-735,-410) and (-250,-230).
2. Central connector: cubic curve from Garden edge (-244,12), controls
   (-231,12), (-218,33), to Abu Dhabi street edge (-205,33). Width 12 m
   at Garden, up to 14 m centrally, tapering to 10 m at the existing street.
3. Khan feeder: T-mouth at new boulevard edge (-199,-420), narrowing
   to 14 m, then flaring to the actual 128-segment outer-loop edge
   around (-140,-420), between ring angles 174.375 and 185.625 degrees.
   The new boulevard center at this junction is (-208,-420).

## Inspected exclusions and connections

- Baiterek plaza X=-50..50, Z=-30..30 and its central pedestrian axis remain
  untouched. Talan Towers and the two eastern buildings stay untouched.
- Northern Lights body X=-130.25..-69.75, Z=-88.25..16.25; its outer
  landscaping reaches X=-231, Z=-108, and parking ends at Z=-12. The
  central connector passes only through the free positive-Z gap beyond it.
- Abu Dhabi Plaza road block X=-205..-58, Z=28..185 is reused, along with
  its existing Ak Orda approach/drop-off. No palace access is relocated.
- Garden street loop X=-496..-244, Z=-266..18 is reused. Local outer
  sidewalk/curb openings are cut to the actual new carriageway footprint;
  the internal garden and colonnade are not changed.
- Arena body/foundation X=-494..-246, Z=-535..-335, plaza to Z=-271,
  parking X=-614..-542, Z=-522..-396.4. New boulevard stays south of the
  foundation at Z=-580 and east of the stadium at X=-208. Arena continues
  to use its approved Garden-side parking connection.
- EXPO's ring and existing Garden connector remain intact. Only the south
  frontage curb/walk/edge marking opens at X=-750..-720, Z=-410. This
  leaves a 6 m gap from the approved MEGA T-mouth ending at X=-756.
- MEGA body, canopy, access, parking and landscaping lie west of the new
  EXPO terminal/spine; no site or building is changed.
- Khan feeder follows the clear west radial axis, avoiding outer tree groups
  at 155/215 degrees and lights at 168.75/191.25 degrees. Only four outer
  curb segments are opened; the inner loop, parking and buildings remain.
- River corridor and waterfront planting/promenenades on positive X remain
  untouched. A future eastern access needs its own pedestrian-safe plan.

No terrain extension is required. No buildings, cars, trees, lamps, parking
or new full-length sidewalks will be added in this stage. Reserve roughly
4-6 m of empty roadside ground for later pedestrian/landscape design.
Geometry counts and final validation will be appended after implementation.


## Built result and validation

All three corridors are implemented. The southern boulevard has 1,038.35 m
of new centerline between existing road edges (approximately 1,050 m between
existing street centers). The central connector is 45.11 m long. The Khan
feeder has about 59.3 m of new asphalt centerline, or 72 m between the new
boulevard and the existing outer-loop center. Combined new reach: about
1,143 m. Body widths are 18 m for the boulevard, 12-14 m tapering to 10 m
for the central connector, and 14 m for the feeder. The boulevard tapers to
12 m before the Garden T. Junction mouths flare wider locally: 30 m at
EXPO, 24 m at the Garden, and approximately 26-27.4 m on the Khan feeder.

The boulevard's suitable sections have a two-metre curbed grass median,
leaving about 8 m of carriageway in each direction, with a dashed lane
separator for two lanes per direction. Median noses taper and the island
opens around the Khan feeder. Lower-speed short links use essential center
and edge markings only. All driving surfaces remain at Y=0.04; paint is at
Y=0.047 and new kerbs at Y=0.14. Existing local-street widths are retained.
Curves use 30 m centerline radii on the boulevard and 6 m curb returns at
its T-mouths. The short central S-curve follows a smooth cubic alignment.

### Junctions and connectivity

Six connected junctions are provided:

| Location | Type / connection |
| --- | --- |
| (-735,-410) | EXPO southern frontage / southern boulevard T |
| (-250,-230) | Garden east street / southern boulevard T |
| (-250,12) | Garden northeast corner / central connector, three arms |
| (-205,33), receiving street block near (-200,33) | Central connector / existing Abu Dhabi road corner, three arms |
| (-208,-420) | New boulevard / Khan feeder T |
| (-136,-420) | Existing Khan outer loop and inner radial road / new feeder, four arms |

This creates a western circulation loop and two previously missing network
links: Garden to the central Abu Dhabi/Northern Lights road block, and
Khan's outer loop to the western streets. EXPO and MEGA already shared a
local network, and EXPO already connected to the Garden; those connections
are preserved, with the new southern boulevard providing another route.
Arena remains served by its existing Garden-side parking road. Baiterek
and Talan retain their approved pedestrian connections. Ak Orda's existing
approach/drop-off connection is preserved; no road crosses its gardens.

The Esil waterfront remains without a new motor-road connection. Reaching
it safely needs a separate eastern corridor plan around the Talan/Baiterek
public spaces and the approved riverside vegetation and paths. No vehicle
road is placed on Atyrau Bridge or either promenade.

### Future development space

The south boulevard provides new frontage for a large empty zone south of
Z=-610 (for example X=-680..-300, Z=-790..-610). A compact empty frontage
parcel remains between the western spine and Arena parking, approximately
X=-705..-635, Z=-540..-435. These are reservations for later planning,
not new geometry or final surveyed building plots. Roadside space remains
available for later sidewalks and planting; no full-length new sidewalk,
building, vehicle, tree, lamp or parking lot is added in this stage.

### Preservation and rendering cost

The new CityRoadNetwork mesh contains **14,353 triangles in four opaque
indexed material draws**, about **1,093,228 bytes** of vertex/index data.
It batches asphalt, stone kerbs, median grass and paint. Junction changes
add 634 triangles in the Garden paving/kerb batches, remove 234 EXPO walk/
kerb/paint triangles and remove 16 Khan outer-kerb triangles. These existing
batches keep their draw counts. The Scene-owned sidewalk remains one cube
draw, with its small clipped corner supplied by the new stone batch.
**Net scene change: +14,737 triangles and +4 draws.**

All road, median, marking and clipped sidewalk geometry is generated once
at initialization and uploaded with STATIC_DRAW. Existing road shaders are
reused without edits. No new transparency pass, per-frame mesh allocation,
geometry sorting or GPU buffer recreation is introduced. Polygon offset,
material flags and depth/blend states are restored after rendering. All
MEGA/EXPO glass caches, stadium seating and parked-vehicle instancing remain
unchanged.

Debug x64 build passed without C++ warnings/errors. The preexisting GLFW
missing-PDB linker warnings remain. A one-run full-scene validation inspected
four 1280 x 720 renders: high aerial, EXPO T-junction, Garden/central street
connection and Khan feeder. Shader compilation, OpenGL error and render-state
checks passed, using Intel UHD Graphics. No FPS claim or benchmark is made.

Uploaded-mesh checks verify finite vertices/UVs, unit normals, valid indices,
nondegenerate winding, continuous route endpoints and the open feeder mouth.
New asphalt has no substantive self-overlap or overlap with old asphalt
(maximum numerical seam overlap approximately 0.0000035 square metres).
The original Garden, EXPO and Khan asphalt triangles are identical; all
non-road meshes in those modules are also identical. Close retained trees,
crowns and streetlights remain clear of the road footprints. All unrelated
working-tree files, shaders, camera code and rendering optimizations retain
their original hashes.

Changed files: src/CityRoadNetwork.cpp/.h, src/CityRoadPlan.h,
src/Scene.cpp/.h, src/BotanicalGarden.cpp, src/ExpoDistrict.cpp,
src/KhanDistrict.cpp, Astana3DTour.vcxproj/.filters and this document.
Only necessary local road/junction edits were made in existing modules.
Stop for F5 visual review. No commit or push.
