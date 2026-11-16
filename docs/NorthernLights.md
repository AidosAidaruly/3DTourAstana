# Northern Lights of Astana — Part 3 commercial podium

Part 3 refines the commercial podium and entrances beneath the approved
Part 2 towers. Earlier stage records remain for context; current podium
details, entrance coordinates and rendering counts are in the final section.

## Selected procedural-world site

The complex replaces the western placeholder pair near Baiterek. Anchor:
**(-100,0,-36)**, rotated **+90 degrees about Y**. Local +Z faces east/+X,
toward Baiterek. This is placement in the existing stylized scene, rather
than a reconstruction from real geographic coordinates.

The actual scene contained four dispersed generic `Building` instances,
not a single three-tower group. The selected plot contained `thirdBuilding`
at (-75,15,0), dimensions 18 x 30 x 14, and `fourthBuilding` at
(-100,20,-35), dimensions 22 x 40 x 16. Both instances and their render calls
are removed. The two unrelated eastern buildings remain at (75,12.5,0)
and (140,17.5,-20).

The shared podium is **104.5 m along world Z x 60.5 m along world X**,
including its projecting upper bands. Bounds: X=-130.25..-69.75,
Z=-88.25..16.25. Podium roof: Y=11.8 m, with three commercial/office levels.
The site, forecourt and connecting path together occupy bounds
X=-138..-50, Z=-96..24. Terrain is unchanged.

## Three-tower composition

| Tower as viewed from the east/front | World X/Z center | Body width/depth in local X/Z | Floors above podium | Total floors | Maximum height including crown |
| --- | --- | --- | ---: | ---: | ---: |
| Left | -101 / -5 | 23 x 26 m | 36 | 39 | 130.22 m |
| Center, set back | -109 / -36 | 24 x 27 m | 41 | 44 | 146.47 m |
| Right | -101 / -67 | 22 x 25 m | 31 | 34 | 113.97 m |

Residential/office floors use a consistent 3.25 m height. The center tower is
8 m behind the outer towers. Clear gaps between their projecting floor bands
are approximately 7.18 and 7.68 m. Bodies remain rectangular, with small
0.7 m corner chamfers and shallow closed roof crowns. All eight facade sides
are generated for every floor; no exposed slab ends or open upper transitions.

The general three-height composition and shared stylobate are informed by
the architectural discussion of Northern Lights in this
[QazBSQA architecture journal](https://vestnik.kazgasa.kz/frontend/web/uploads/archive/doc/1608188198_IYi48v.pdf).
The project uses original procedural geometry and scene-scaled proportions,
not imported geometry or an exact photographic/surveyed replica.

## Part 1 facade and podium

- Turquoise/green-blue opaque reflective glazing, with inexpensive sky tint,
  restrained Fresnel/specular highlights and fine vertical curtain-wall lines.
- Actual projecting light-metal bands at every tower floor; broad daylight
  fill keeps their undersides readable from pedestrian views.
- A darker three-level glazed stylobate, two upper horizontal divisions,
  clean roof edge and ground-level architectural pilasters.
- Three framed glazed entrance reservations, with double-door framing,
  handles and shallow canopies. The central opening is 6 m wide; the outer
  two are 4 m. Glazing is reused across these fields, without duplicate
  transparent walls. Thresholds meet forecourt paving at Y=0.12.
- No interiors, detailed roof mechanics, signage or full parking development.

## Basic site integration

Perimeter paving and the east-facing forecourt use metre-based staggered
stone joints. Paving gently descends from Y=0.12 at the podium to Y=0.06 at
the plot perimeter. Embedded edge faces finish it against the original grass.

Four planted strips retain the original ground, with low stone borders,
four compact deciduous trees reusing the existing Tree crown arrangement,
and eight low shrubs. Entrances and walking routes remain open.

At world Z=24, the perimeter paving meets the existing Abu Dhabi district's
outer sidewalk (Z=24..28), beside its unchanged road at Z=28..38. A 10 x 10 m
pedestrian branch joins the original Baiterek plaza at X=-50, Z=-5..5,
matching its Y=0.05 surface. No new vehicle road or parking is necessary for
this stage; neither the existing road nor its curb alignment is modified.

The podium is at least 19.75 m from Baiterek's nearest plaza edge and about
11.75 m from the nearest existing road asphalt edge. Only the intended paving
connections touch the existing sidewalk/plaza. Talan Towers, Abu Dhabi Plaza,
Botanical Garden and other landmarks remain in their original positions.

## Rendering cost and verification

Seven static indexed material batches share one VAO/VBO/EBO:

| Material | Triangles |
| --- | ---: |
| Tower glazing | 1,728 |
| Podium glazing | 48 |
| Light-metal bands, podium and framing | 7,876 |
| Dark metal/crowns | 294 |
| Site paving and embedded edges | 98 |
| Tree trunks | 48 |
| Tree/shrub foliage | 1,920 |
| Total | **12,012** |

21,222 vertices and 823,248 bytes of vertex/index storage. The removed pair
previously submitted 2,040 triangles in 170 cube/window draw calls. Calculated
net scene change: **+9,972 triangles, -163 draw calls**. These are geometry
submission counts, not FPS measurements. There is no new transparency pass,
sorting, per-frame generation, allocation or GPU buffer rebuilding.

Debug x64 build passed, with only preexisting GLFW missing-PDB linker warnings.
Initialization validates finite positions/unit normals, indices, nondegenerate
triangles and winding. Placement checks confirm separated towers, complete
floor-ring generation and no overlap with the inspected road/sidewalk and
landmark footprints. Removing the Northern Lights shader uniform/branch leaves
the previous shared shader unchanged. Glass caches, seat/vehicle instancing,
all controls and unrelated working changes remain untouched.

Final 1280 x 720 full-scene views inspected:

- Elevated: eye (85,180,135), target (-90,50,-35).
- Medium street view: eye (70,3,-36), target (-100,65,-36).
- Close facade: eye (-52,3,-36), target (-95,16,-36).

Shader compilation, OpenGL error checks, depth/blend state and material flag
restoration passed. The hidden validation context used Intel UHD Graphics.
No performance benchmark suite or new visible FPS claim is included.

Changed files: new src/NorthernLights.cpp/.h, src/Scene.cpp/.h,
shaders/ground.frag, both Visual Studio project files, and this document.
Part 1 only; stop for F5 review. No commit or push.

## Part 2 — Turquoise curtain walls and architectural finishing

### Approved geometry preserved

Anchor (-100,0,-36), +90-degree Y rotation, all three tower body footprints,
podium footprint and site connections remain unchanged. Maximum tower heights
are still 130.22 / 146.47 / 113.97 m, with 39 / 44 / 34 total floors.
The center remains tallest and set back. Existing podium glazing, entrances,
landscape, paving, roads and neighboring landmarks are preserved.

### Glazing and horizontal bands

- Each tower face now receives a complete, equally spaced curtain-wall grid.
  Typical bay width is approximately 1.8 m, adjusted slightly to divide the
  actual facade length. All four main facades and chamfered corners are covered.
- Opaque turquoise glass uses restrained blue-green coating shifts, smooth
  sky/horizon reflections, Fresnel response and modest directional highlights.
  Slightly darker spandrel zones and fine filtered transoms provide facade
  depth without a square apartment-window pattern or random colored patches.
- All **108 tower-floor bands** are rebuilt as closed, shallow beveled silver
  profiles, replacing the original simple ledges. Their largest outer ring is
  the approved Part 1 ring, so the envelope does not grow.
- **95 standard bands are 0.20 m high; 13 structural bands are 0.28 m high**,
  selected at eight-floor intervals. The 3.25 m storey pitch is unchanged.
- The three roof-base rings and three crown trims receive the same bevel
  treatment. Corners are joined using shared contour samples throughout.

### Profiles and crowns

- **142 secondary vertical mullions** use 50 mm-wide, 56 mm-deep dark profiles,
  attached directly to the glazing and recessed behind the 160 mm projecting
  floor edges. They are continuous rather than separate objects per window.
- **24 corner frames** use restrained 80 mm sections, following the chamfer
  junctions and joining the podium/tower base to the crown.
- Crown parapets have two flush light-metal reveals per tower, a closed roof
  deck and refined coping. No tower roof is raised or enlarged.
- **Six compact rooftop service units**, two per tower, have simple louvered
  caps. They stand on the approved roof deck and remain below the existing
  technical cap's maximum elevation. The original main technical caps remain.

Very fine 3D mullions/corner profiles use opaque near-detail visibility within
65 m; farther away, antialiased seams on the glazing preserve the divisions.
This prevents broken subpixel dark strokes in city views. A dedicated negative
UV marker identifies only these profiles; original podium metal and roof caps
are unaffected. The change is shader-only visibility, with no CPU LOD grouping,
buffer updates, alpha blending or transparency sorting. All geometry remains
static and submitted in the same material batches.

### Cost and validation

| Material | Part 2 triangles |
| --- | ---: |
| Tower glazing | 1,728 |
| Podium glazing, unchanged | 48 |
| Silver bands, trim, podium and service-unit bodies | 11,692 |
| Dark metal, mullions, corner profiles and louver caps | 2,742 |
| Site paving, unchanged | 98 |
| Tree trunks, unchanged | 48 |
| Foliage, unchanged | 1,920 |
| Total | **18,276** |

**Added: 6,264 triangles and zero draw calls.** The complex remains seven
static opaque draws. There are 33,750 vertices and 1,299,312 bytes of
vertex/index storage, an increase of 476,064 bytes. No new textures or external
dependencies are used. No per-frame geometry, sorting or GPU buffer rebuilding.

Debug x64 build passed with no new compiler warnings/errors; the existing
GLFW missing-PDB linker warnings remain. Mesh initialization validates finite
positions, normals and UVs, unit normals, indices, nondegenerate triangles and
winding. Checks confirm all floor rings, 13 heavier bands and 166 vertical
profiles, unchanged individual tower heights and plan bounds, and exact
preservation of the podium/site triangle data. The podium glass calculations
and all shared-shader code outside Northern Lights remain unchanged.

Final full-scene 1280 x 720 renders inspected:

- Elevated three-quarter: eye (85,180,135), target (-90,50,-35).
- Street view of all three towers: eye (70,3,-36), target (-100,65,-36).
- Close tower curtain wall: eye (-74,28,1), target (-99,36,-5).

Shader compilation and OpenGL error/state checks passed, including depth
writes/testing, blending and material-flag restoration. The validation context
used Intel UHD Graphics. No new FPS measurement or benchmark suite was run;
the user's approximately 124 FPS visible baseline remains for F5 comparison.

Reference photographs were not accessible as local project assets. The work
follows the supplied architectural description; exact photographic agreement
is not claimed. Reflections remain an efficient opaque sky approximation.

Changed for Part 2: src/NorthernLights.cpp/.h, shaders/ground.frag and this
document. Scene, project configuration, controls, stadium seat instancing,
MEGA/EXPO glass caches, vehicle LOD and unrelated working files are unchanged.
Stop for F5 visual review. No commit or push.

## Part 3 — Detailed shared commercial podium

### Orientation and retained envelope

The building anchor remains (-100,0,-36), with the approved +90-degree Y
rotation. Local +Z faces Baiterek/east (+X in the world). Local -X faces
the existing city road/south (+Z in the world). The principal entrance is
on the Baiterek-facing frontage near the existing plaza connection; an
additional secondary portal serves the road-side sidewalk directly.

The podium mass still occupies 104.5 x 60.5 m including projecting bands,
with a general roof height of 11.8 m. Roof deck and tower bases are unchanged.
All three complete tower meshes, 108 floor bands, 142 mullions, 24 corner
frames, crowns, technical units, dimensions and maximum heights are preserved.

### Commercial facade

- The original plain continuous podium glazing is replaced by **159 framed,
  recessed commercial/office bays**: 51 at ground level and 54 on each upper
  level. Four recessed entrance fields are separate from this count.
- All four long/short sides and their corner chamfers are finished. Silver
  corner joints close the small offsets between adjoining facade planes.
- Alternating 0.18 / 0.32 m glazing setbacks create real depth. Thin aluminum
  jambs, horizontal trims and finished reveals enclose every glazed field.
  There is no retained solid wall directly behind the new frontage.
- Darker blue-green opaque glazing has restrained sky/Fresnel highlights,
  fine filtered pane divisions and subtly distinct ground/upper-level tones.
  No new transparency or sorting pass is used.
- Two full-width beveled intermediate silver bands articulate the levels.
  A closed dark roof-edge reveal and silver coping finish the upper rim,
  remaining inside the approved podium plan and roof elevation.

### Main and secondary portals

Door centers below refer to the recessed glazing at threshold level, not
the outer edge of the canopies. Every threshold is at Y=0.12, meeting the
unchanged adjacent paving without a step.

| Portal | World X / Y / Z | Facing | Double-door groups |
| --- | --- | --- | ---: |
| Main, near the Baiterek approach | -70.90 / 0.12 / -5 | +X | 5 |
| Middle frontage secondary | -70.65 / 0.12 / -36 | +X | 2 |
| Opposite frontage secondary | -70.65 / 0.12 / -67 | +X | 2 |
| City-road secondary | -100 / 0.12 / 15.35 | +Z | 2 |

There are **11 double-door groups / 22 door leaves** overall. Door groups
are 2.4 m wide, with center and side stiles, transoms, low kick plates and
paired handles. The main portal is 16.8 m wide and recessed 0.90 m; secondary
portals are 6.4 / 6.4 / 7.0 m wide with 0.65 m recesses. Finished side piers,
soffits and shallow stone threshold floors enclose the vestibule geometry.
Old small entrance/canopy geometry is replaced, rather than layered behind it.

Four new metal-framed, blue-silver glazed canopies cover these portals. They
have a slight outward rise, finished undersides and actual longitudinal ribs.
The main canopy is approximately 18.3 x 4.42 m, projecting 3.46 m beyond the
facade, including trim. The two frontage secondary canopies are approximately
7.5 x 3.27 m; the road-side canopy is 8.1 x 3.27 m. The latter project only
2.56 m beyond the facade. Their lowest structural underside is about Y=3.40,
above the application's 3 m walking-camera eye. No freestanding columns block
the forecourt. The tinted canopy surfaces are efficiently opaque and share
the existing podium-glass batch.

The new threshold paving adds only about 27.14 square metres inside the
podium's facade recesses. Original paving, lawns, trees, shrubs and pedestrian
links are retained. Canopies clear the existing planting and stay within the
site; the road-side projection leaves over 5 m of the existing approach walk.
No road, curb or surrounding sidewalk is moved or rebuilt.

### Rendering and validation

| Material | Part 3 triangles |
| --- | ---: |
| Approved tower glazing, unchanged | 1,728 |
| Commercial, entrance and canopy glazing | 370 |
| Silver cladding, frames, trim and existing tower bands | 14,732 |
| Dark metal, including unchanged tower profiles | 3,438 |
| Paving, including recessed thresholds | 106 |
| Tree trunks, unchanged | 48 |
| Foliage, unchanged | 1,920 |
| Total | **22,342** |

Added **4,066 triangles and zero draw calls**. The whole complex remains
seven static indexed opaque material draws. It now has 41,882 vertices and
1,608,328 bytes of vertex/index storage (309,016 bytes more than Part 2).
No per-shop or per-door draws, per-frame geometry updates, dynamic reflections,
transparency passes or GPU buffer recreation were introduced.

Debug x64 build passed, with no new compiler warnings/errors. Preexisting
GLFW missing-PDB warnings remain. Initialization validates finite positions,
normals/UVs, indices, nondegenerate triangles and winding. Geometry comparisons
confirm all approved tower triangle data and original landscape are unchanged;
the original paving is retained with only four internal threshold quads added.
Checks confirm all eight podium facade directions, unchanged body envelope,
clear canopy/planting separation and level threshold connections. Shared shader
code outside the podium-glass branch is identical to Part 2, including tower
glazing and the near-detail profile policy.

Final full-scene 1280 x 720 views inspected:

- Baiterek-facing frontage: eye (35,4,-36), target (-84,5,-36).
- Elevated three-quarter: eye (85,180,135), target (-90,50,-35).
- Main entrance close-up: eye (-57,3,-1), target (-71,2,-5).

Shader compilation, OpenGL error checks, depth/blend state and material flag
restoration passed. This hidden context used Intel UHD Graphics; no new FPS
measurement or performance benchmark is claimed. Glazing remains an opaque
reflection approximation and doors are static. No shop interiors are included.

Changed for Part 3: src/NorthernLights.cpp, shaders/ground.frag and this document.
Scene, header, project files, controls, glass caches, seat/vehicle instancing
and all unrelated working-tree changes remain untouched.
Stop for F5 visual review. No commit or push.

## Part 4 — Public realm and compact west parking

The approved three towers, podium, all four entrances, original paving, four
existing trees, eight shrubs and four planting strips are retained exactly.
No terrain extension or new raised terrain surface is used.

### Paving and pedestrian connections

- An 8 m-wide extension of the eastern frontage walk occupies X=-60..-52,
  Z=-96..-5 and Z=5..24. Four 3 x 6 m garden pockets leave a continuous
  5 m newly paved strip beside the pockets, in addition to the original
  front walk. Edge furniture leaves at least 4.5 m of combined clear width.
- A 98 x 12 m end court at X=-158..-60, Z=-108..-96 connects to the original
  podium perimeter walk. Four planting pockets and narrow charcoal edge
  inlays articulate it. The paving is at Y=0.06, with no raised platform.
- Two 4 m-wide paths join the west parking walk to the podium's existing
  rear walk at X=-138: Z=-68..-64 and Z=-16..-12. The latter crosses the
  new driveway on a marked zebra crossing. A 10 x 16 m sitting bay at
  X=-148..-138, Z=-58..-42 also meets the rear walk.
- The original broad Baiterek connection at X=-60..-50, Z=-5..5 remains
  completely open. All entrance thresholds, canopies and existing paths
  remain unchanged. Rectangular stone joints reuse the existing local
  paving shader, with no per-tile geometry or draw calls.

### Landscape and furniture added

| Item | Added |
| --- | ---: |
| Trees | 16 |
| Low shrubs | 34 |
| Ornamental grass clusters | 28 |
| Bordered flowerbeds | 10 |
| Low flower clusters | 288 |
| Wood-and-metal benches | 14 |
| Pedestrian lights, unlit | 20 |
| Waste bins | 10 |

There are now 20 trees and 42 shrubs including the original planting.
Four slender trees frame the eastern forecourt; four medium trees occupy
end-court beds; five shade the parking divider/perimeter; three occupy the
western green strip. Crowns are approximately 1.4-1.9 m in radius and trees
are about 7.2-8.0 m tall. Flowerbeds use restrained red, ivory, yellow and
purple bands, eight-triangle low flower mounds, low shrubs and grass tufts.
All trunks and crowns stay clear of the walking routes and parking stalls.
The westernmost bed is 8 m east of the Botanical Garden's outer sidewalk.

Bench and StreetLight component transforms are reused, baked into shared
opaque batches once. Lights are approximately 5.4 m tall and emit no light.
Furniture stays out of the original central entrance approach; planting and
furniture do not occupy doors, canopies, road lanes or accessible bays.

### Parking and street connection

- **62 spaces: 60 standard and 2 accessible.** No cars added.
- Standard bays are 5.0 x 2.6 m; accessible bays 5.0 x 3.4 m with a shared
  1.6 m transfer aisle and adjoining protected pedestrian waiting area.
- Four rows occupy two double-loaded modules with 7 m two-way aisles,
  8 m end headers and a 10 m planted central divider.
- Outer parking walks: **X=-218..-166, Z=-86..-20.4**, about **52 x 65.6 m**.
  Marked bays lie within X=-214..-170, Z=-74..-32.4.
- A 7 m-wide driveway follows X=-182..-175, Z=-24.4..23, flaring to 9 m at
  the existing street's northern edge, centered **(-178.5, 0, 28)**.
- Only the existing sidewalk at Z=24..28 and curb at Z=28..28.3 receive
  a local opening at X=-183..-174. Existing road asphalt, alignment,
  crossings and centerline markings remain unchanged. Elsewhere the
  original sidewalks/curbs are identical. The crossing path within the
  site is 4 m wide, at Z=-16..-12, away from the public-road junction.

Parking asphalt has a small, isolated opaque procedural variation branch
in the existing shader. Tower, podium, paving, metal and all unrelated
shader branches are unchanged. No additional transparent geometry exists.

### Rendering cost and checks

Northern Lights now submits **38,546 triangles in 13 opaque indexed draws**,
up from 22,342 triangles and 7 draws. Added geometry: **16,204 triangles,
6 draws**, with 853,904 additional vertex/index bytes. Splitting the two
existing sidewalk/curb rectangles adds **24 triangles and 2 existing cube
draws**. Total scene impact: **16,228 triangles and 8 draws**.

All geometry is created during initialization and uploaded with STATIC_DRAW.
No per-object draws, per-frame allocations, updates, new transparency passes,
dynamic lights or reflection rendering were introduced. Glass caches,
seat instancing and vehicle LOD/instancing are untouched.

Debug x64 builds successfully. Mesh initialization validates finite vertices,
unit normals, valid indices, nondegenerate triangles and consistent winding.
Checks confirm all original seven material batches are retained triangle for
triangle, old/new paving has no overlapping areas, every bay is valid and
unobstructed, planting stays off paths and the original main approach is clear.
Scene changes are limited to the local driveway opening; shader changes are
limited to the new parking material. Full-scene OpenGL checks confirm shader
compilation, no GL errors and restored depth/blend/material states.

Inspected 1280 x 720 views:
- Elevated site/road overview: eye (-280,125,110), target (-145,15,-35).
- Baiterek-facing entrance: eye (-40,3,-1), target (-71,3,-5).

The check context used Intel UHD Graphics. No FPS benchmark was run, and no
new visible-F5 FPS claim is made. Plants use economical stylized opaque
geometry. No parking cars, gates, shop interiors or dynamic lighting added.

Changed: src/NorthernLights.cpp/.h, src/NorthernLightsSite.h, src/Scene.cpp,
shaders/ground.frag, Astana3DTour.vcxproj/.filters and this document.
All unrelated working-tree changes are preserved. Stop for F5 review;
no commit or push.
