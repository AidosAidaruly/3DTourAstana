# City Development Part 5A - Background city expansion

## Automatic city infill (no roads)

Adds **90 buildings in eleven previously vacant envelopes**: three outer
west EXPO groups, two northern EXPO extensions, two EXPO/Garden outer gaps
and four southern housing/Khan outer groups. Exact envelopes and staggered
placement parameters are in `BackgroundCityExpansionPlan::automaticAreas`.
Six existing designs provide 6-16 floors (21.6-56.6 m); minimum body gap is
17 m. Existing 197 placements and their courtyard reservations are retained;
the shared background system now contains 287 buildings in 45 groups.
Only entrance aprons are added. No road, parking, landmark or shader edits.
Existing static instancing, three LODs and cached culling are unchanged.
Added near/medium/far cost: 732,110 / 211,878 / 117,270 triangles, 44 draws
when all eleven groups are visible, and 961,656 GPU bytes. Debug x64 and
placement/mesh/OpenGL checks pass. Two aerials show all 90 additions (58/32),
and a street-level facade was inspected. An initial EXPO aerial sample with
33 additions visible measured 87.1 FPS in hidden Debug 1280x720 on Intel UHD;
this is not a matched performance comparison. Protected parks, landmark
grounds, public axes, riverfront and existing courtyards remain open.

## Eastern field density pass (no roads)

The previously unserved field is now developed at the user's explicit request:
**45 buildings in six groups**, entirely X=157.7..302.7, Z=140.7..431.7.
Six existing designs provide 6/8/10/12 floors plus one 14-floor and one
16-floor accent (22.0..55.5 m). All 152 previous placements remain unchanged.
Only tiled building aprons are generated: no roads, driveways, parking or
external path connections. Palace gardens, C-G1 and the river buffer stay open.
Minimum building gap: 15.05 m. Existing instancing, three LODs and culling stay.
Added near/medium/far cost: 328,762 / 98,130 / 56,018 triangles, 31 draws and
804,888 GPU bytes. Both requested aerials visibly contain the new blocks.
Debug x64, placement, normals/winding and OpenGL checks pass. One brief hidden
1280x720 Intel UHD sample measured 69.5 FPS; no matched baseline is claimed.
Earlier sections below describe previous milestones, including land left
vacant at those stages. No completed road or landmark source was modified.

## Further central infill

Adds **27 buildings in four groups**, bringing the background system to
152 buildings / 28 groups. All 125 earlier placement records are unchanged.
No new archetypes, rendering framework, roads or landmark edits are required.

| Group | World X / Z bounds | Buildings | Use and access |
| --- | --- | ---: | --- |
| 24 | 50..142 / -237..-122 | 8 | 6-12-floor residences; C2 west kerb at (149.8,-178.25) |
| 25 | 212..287 / -579..-438 | 8 | Six residences, two office/mixed-use buildings, 4-14 floors; C1 south kerb at (250,-425.2) |
| 26 | -226..-151 / 205..365 | 10 | Two residences, five offices, three retail buildings, 3-8 floors; retained Abu Dhabi north sidewalk at (-188.5,189) |
| 27 | 112..144 / -273..-245 | 1 | 3-floor corner retail; C2 west kerb at (149.8,-258) |

Overall: 16 residential / 7 office-mixed / 4 commercial, approximately
12.0-48.8 m high. Minimum body separation is 14.5 m. Four connected pedestrian
courts include small lawns, five trees, ten shrubs and one bench. Lower
3/4-floor buildings face the Ak Orda side; their aprons remain at least
29.5 m outside its protected X=-125 boundary. Existing roadside trees remain.

The eastern palace hinterland has no suitable vehicle connection and is
intentionally vacant. C-G1, the pedestrian axis, ceremonial gardens, bridge
approach and X=315..330 river buffer remain open. No riverfront infill is forced.

Existing instancing, three LODs, cached parcel culling and static buffers are
reused. Added cost with every new group visible: 155,398 / 47,062 / 27,322
triangles at near / medium / far LOD, including 3,080 site triangles; 19 draws
and 392,760 GPU bytes. The inspected Esil aerial shows 17 additions at mixed
medium/far LOD: 27,710 triangles / 14 draws.

Debug x64 build, placement bounds, road clearances, normals/winding and full
scene OpenGL checks pass. Esil and Ak Orda aerials plus a street view were
inspected. A short same-context draw-toggle test at Debug 1280x720 on Intel
UHD measured 75.9 FPS without the additions and 78.9 with them; timing noise
prevents a reliable FPS delta or performance-gain claim. No visible F5 result
is claimed. Artifacts are ignored under obj/central-infill-check.

## Part 6B - central district addition

The same background renderer now also contains **25 central buildings** in
four approved parcels (125 background buildings / 24 groups in total).
The original 100 placements, 18 building LOD meshes and 20 site meshes are
unchanged. No roads, landmarks, scene shaders or camera controls are edited.

| Parcel | World X / Z bounds | Buildings | Floors / use |
| --- | --- | ---: | --- |
| C-P1 | 212..287 / -402..-253 | 10 | 6-12, residential courts; taller western row |
| C-P2 | 169..215 / -227..-143 | 3 | 6, offices with glazed ground floors |
| C-P3 | 231..287 / -227..-143 | 6 | Four 3-floor retail buildings, two 4-floor business buildings |
| C-P4 | 196..287 / -117..-33 | 6 | Five 8-10-floor residences, one 6-floor mixed-use building |

Overall mix: 15 residential / 6 office or mixed-use / 4 retail. Heights are
approximately 12.0-42.1 m. Footprints are 14-20 m wide and 12-20 m deep,
with at least 14.5 m between building bodies and 2.5 m entrance aprons.
Four 4 m pedestrian spines connect to retained kerbs at (250,-414.8),
(209,-234.8), (259,-234.8), and (227,-124.8). Courtyard paving, small lawn
strips, three trees, six shrubs and one bench complete the immediate sites.
The X=215..231 middle green strip, C-G1 bridge-view space, central pedestrian
axis, river buffer and all existing public roads remain clear.

Six existing archetypes are retained; three shared 3/4/6-floor variants use
the same facade generator, opaque materials, immutable instance buffer,
three LODs, parcel-frustum culling and existing LOD hysteresis. Low-rise-only
setback/corner handling prevents negative-height framing. No buffer uploads
or mesh generation occur during rendering. With all four parcels visible,
the addition costs 141,358 / 42,798 / 24,798 triangles at near / medium / far
LOD, including 2,600 site triangles, and **14 draws**. Additional GPU storage:
1,459,440 bytes. The inspected aerial uses far LOD for all 25 new buildings.

Debug x64 build, initialization placement/mesh validation, full-scene shader
compilation, GL errors/state restoration, aerial and pedestrian views pass.
An isolated hidden Debug 1280x720 Intel UHD Graphics sample measured 51.6 FPS;
this is not a matched before/after comparison or a visible F5 performance
claim. Verification artifacts stay ignored in obj/central-development-6b-check.

The following sections retain the original Part 5A milestone documentation.

Adds **60 buildings in ten six-building groups**. Together with Part 4,
the shared background renderer now contains 100 buildings in 20 groups.
All earlier landmark and residential geometry remains intact.

## Selected land and access

World coordinates are X/Z metres. These are building envelopes, not new
raised terrain slabs. Exact centers, sizes, colors and orientations are in
`src/BackgroundCityExpansionPlan.h`. Building bodies have at least 16 m
separation across both background phases. Each group contains six buildings.

| ID | Area | X bounds | Z bounds | Connection to retained paving |
| --- | --- | --- | --- | --- |
| 10 | EXPO southwest outer corner | -1037..-969 | -375..-232 | Inner west-boulevard sidewalk at (-1048.6,-326) |
| 11 | EXPO west inner frontage | -1047..-997 | -128..-8 | Inner west-boulevard sidewalk at (-1048.6,-88) |
| 12 | Arena northwest / EXPO corner | -660..-508 | -382..-202 | Arena access walk (-568.25,-350); upper subgroup uses Garden walk (-501,-240) |
| 13 | Arena west frontage | -713..-630 | -548..-434 | EXPO south sidewalk at (-715,-421.4) |
| 14 | Khan northwest / south of Northern Lights | -190..-65 | -250..-126 | Garden east outer sidewalk at (-239,-163) |
| 15 | Garden outer west | -500..-438 | 214..488 | West edge of N2 at (-424.2,230) |
| 16 | Garden outer east | -274..-235 | 205..488 | East edge of N2 at (-275.8,230) |
| 17 | EXPO western courtyard extension | -1470..-1250 | -320..-190 | Existing Part 4 entrance apron at (-1198.5,-294) |
| 18 | EXPO northwest courtyard extension | -1045..-870 | 355..505 | Existing Part 4 rear apron at (-1006,312.5) |
| 19 | EXPO northeast courtyard extension | -850..-690 | 355..505 | Existing Part 4 rear apron at (-809,310.5) |

Groups 17-19 use continuous pedestrian connections into existing residential
walks, which already connect to the boulevard. They do not gain new public
carriageways or parking in this stage. Garden-side housing similarly extends
along connected campus paths beyond the street loop. No isolated plots,
new highways or altered public road alignments are introduced.

The northwest Arena plot follows the actually empty wedge around its
parking-access dogleg. The former broad bounding reservation was refined
into actual foundation, plaza, parking and access-road protection. The
parking/access generator supplies conservative bounds for all retained
asphalt and stone pieces. Buildings and their aprons remain clear of them.
The two upper subgroups connect separately to avoid crossing this road.

EXPO's protected circle remains clear by at least the R230 planning envelope.
The narrow west frontage uses compact 12 x 18 m towers rather than intruding
into the ring. Group 13's connection approaches its sidewalk perpendicularly,
clear of the existing arterial T-junction. No existing paving is covered.

## Buildings and site treatment

The SAME six shared archetypes serve both phases:

| Archetype | New count | Floors | Character |
| --- | ---: | ---: | --- |
| A10 | 15 | 10 | Ivory apartment slab |
| A12 | 15 | 12 | Warm stone apartment wing |
| C14 | 10 | 14 | Gray/glass residential or mixed-use block |
| C16 | 10 | 16 | Taller glass-accented block |
| B22 | 6 | 22 | Compact brick-and-stone tower |
| D8 | 4 | 8 | Lower warm-brick housing |

Heights are **27.86..78.25 m**. Footprints vary approximately 12..28 m in X
and 18..26 m in Z; orientation and nonuniform horizontal instance scales
preserve realistic floor heights. Restrained per-instance warm/cool colors
supplement the existing material and window-pattern differences. Each new
group uses only three archetypes, enabling repeated instanced draws.

Facade depth, floor bands, dark window frames, selected loggias, glazed lobby
frontages, entrance canopies, two roof setbacks and parapets come from the
existing medium-detail generator. No unique building mesh is generated for
each placement. There are no interiors or new transparency passes.

Ten lightweight site compositions add **30 trees, 60 shrubs and four benches**,
small lawns and approximately 25,383 square metres of combined entrance
aprons and 4 m pedestrian branches. Tree crowns, shrubs and bench footprints
are checked against building envelopes, paths and existing road surfaces.
Connections match Y=.06 paving or Y=.14 retained kerbs. The old forty
buildings' placements, all eighteen shared meshes and all ten old site
vertex/index buffers are unchanged, with byte comparisons confirming this.

## Rendering cost and optimization

Eighteen existing indexed meshes (six archetypes x three LODs) remain shared
by all 100 instances. A new static X/Z scale attribute and restrained tint
parameters provide variation. Normals use inverse scale before rotation.
No additional building-template buffers are required. The instance buffer
contains 100 x 40 bytes. Every group/type/LOD VAO refers to a fixed range;
visibility or LOD changes select VAOs without regrouping or uploading data.

The original three LODs, 90/130 m and 245/300 m hysteresis, cached camera
decisions and per-group frustum culling remain. All geometry is initialized
once. Site materials share one draw per group, with no per-object draws.

| Entire new expansion visible | Buildings | Sites | Added total | Added draws |
| --- | ---: | ---: | ---: | ---: |
| Near LOD0 | 671,744 | 12,394 | 684,138 | 40 |
| Medium LOD1 | 184,712 | 12,394 | 197,106 | 40 |
| Far LOD2 | 97,192 | 12,394 | 109,586 | 40 |

These are full-set LOD bounds, not a claim that every building simultaneously
uses near LOD in a real view. Far building geometry is 85.5% lighter than
near geometry. All 100 at far LOD submit 187,244 triangles / 83 draws.
Static GPU storage is 11,717,456 bytes, **+1,572,488 bytes** over Part 4.

## Validation and performance

Debug x64 builds successfully without new C++ warnings. Full-scene runtime
checks pass shader compilation/linking, OpenGL errors, depth/blend/material
flags, generic instance attributes and polygon-offset restoration. All LOD
meshes have finite vertices, unit normals, valid indices and nondegenerate
outward winding. Instance scales, protected areas, original residential
parcels, road clearances and body separation are checked at initialization.

Uploaded-geometry checks found no substantive overlap with existing roads,
sidewalks, old courtyard paving or building bodies, and no duplicate new
paving. Maximum numerical edge overlap is 0.0000123 square metres. All old
shared mesh/site buffers are byte-identical. Every unrelated source retains
its starting hash, including roads, landmarks, camera, shaders, glass caches,
vehicles and stadium seats. No terrain or camera controls are changed.

Inspected a high aerial showing the expanded city, three street-level views
(Arena northwest, Garden outer housing and an EXPO corner), and an elevated
Arena-facing view. Three LOD selections and group rejection were observed
after camera relocation. The aerial capture alone uses a wider validation
FOV/far plane; normal game projection is unchanged.

One matched Debug x64 / hidden 1280 x 720 / Intel UHD Graphics comparison,
eye (-1360,180,-90), target (-700,25,-260), normal projection, vsync off,
12 warm-up frames and 120 GPU-synchronized measured frames:

| Sample | FPS | Frame time |
| --- | ---: | ---: |
| Before | 98.35 | 10.17 ms |
| After | 94.09 | 10.63 ms |

Frame time increases approximately **4.5%** in this sample. The after view
contains 72 background buildings: 18 medium / 54 far, 163,030 triangles and
59 draws. Before, the module submitted 24 buildings, 63,826 triangles and
27 draws. These are actual short offscreen measurements; visible F5 results
and moving-camera performance remain for the user's review.

## Intentional open areas and changed files

Botanical Garden's interior, riverfront, landmark axes, stadium plaza,
parking, EXPO circle, MEGA grounds and Khan's landscaped ring stay clear.
Land farther outside the connected network remains undeveloped. No attempt
was made to force buildings into those reservations or disconnected fields.

Added: src/BackgroundCityExpansionPlan.h and this document.
Modified: src/BackgroundResidential.cpp/.h, Astana3DTour.vcxproj/.filters
and docs/BackgroundResidential.md (a link to this phase).
Scene integration, the original Part 4 plan and all other modules are intact.
Ignored validation artifacts are in obj/background-city-5a-check.
Stopped for F5 visual review. No commit or push.
