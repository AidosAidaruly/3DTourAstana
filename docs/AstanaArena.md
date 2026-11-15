# Astana Arena — completed stadium and surrounding district

Current facade: Part 2.5 supersedes the office-like Part 2 treatment. The
Part 3 roof silhouette is preserved, with Part 3.5 mechanism and lining
finishes preserved above the new short collar. Part 4 adds the main entrance;
Part 5A introduced the approved ground-level pitch and bowl foundation.
Part 5B adds instanced blue/white seating and circulation. Parts 6A–6C add the
approved entrance plaza, west-side parking, landscaping and urban furniture.
The completed milestone has been reviewed in F5. Earlier sections retain
their historical validation data; current district details are at the end.

## Placement

- Center: **(-370, 0, -435)**, unchanged. The complete stadium is rotated
  **+90 degrees about Y**, so its 240 m long axis is now parallel to world X.
  Its broad long side faces the Garden and existing road on the +Z side.
- Stadium envelope: **240 m X × 196 m Z**, bounds X=-490..-250,
  Z=-533..-337. Maximum roof elevation **Y=39.746 m**.
- Oval foundation: **248 m X × 200 m Z**, bounds X=-494..-246, Z=-535..-335.
  Its bottom is Y=-0.12 and top Y=0.28, seated in existing flat Y=0 terrain.
- The curved colonnade midpoint is approximately (-370, 0, -214.5), inside
  the Garden. The parcel is beyond its southern perimeter road, in the
  negative-Z direction toward Khan Shatyr, rather than in EXPO or MEGA.
- The foundation is 64 m from the Garden's nearest outer sidewalk at Z=-271
  and 69 m from its asphalt edge at Z=-266. There is a 120.5 m axial gap to
  the colonnade midpoint. Existing trees stay on the Garden side of the road.
- Conservative coordinate-envelope separation exceeds 85 m from the Khan
  district's outer road/parking and 120 m from the EXPO circular sidewalk.
  No existing road, path, tree or structure was moved or regenerated.
- The existing terrain covers the entire site; no terrain extension or
  separate rectangular preparation surface was added. Land outside the
  oval foundation remains available for later public space and circulation.

The user approved this location and scale in the Part 1 F5 review, then
requested the current 90-degree orientation correction. The four reference
images were not accessible locally; the facade
uses the supplied architectural description rather than claiming exact
photographic agreement.

## Part 1 baseline geometry

The original procedural shell has a tapered lower body, broad continuous
oval facade, layered eaves, and a gently raised silver roof shoulder.
A distinct **88 × 140 m** rectangular closed placeholder reserves the future
retractable-roof section, aligned to Z. No detailed facade, roof mechanics,
seating, entrance, new road, parking or landscaping is included.

All shell layers share 196 angular stations, including the exact rectangular
roof corner rays. The roof shoulder uses 20 radial segments. Connected
boundary rings and cap fans form a closed shell without overlapping caps.

- 8,038 vertices; **11,760 triangles**; **5 opaque material batches/draws**.
- Static GPU vertex/index storage: **398,336 bytes** (approximately 389 KiB).
- Geometry, validation and GPU uploads run once in the constructor.
- Rendering reuses the existing ordinary opaque shader. No shader changes,
  new transparency, sorting, LOD changes or per-frame geometry allocations.

## Part 1 verification

Debug x64 build succeeded. Existing GLFW missing-PDB LNK4099 warnings remain;
no new compiler warnings or errors were reported.

Initialization checks finite vertices, unit normals, index bounds,
nondegenerate triangles and outward winding. An independent welded-edge
check confirmed every shell edge has two oppositely oriented incident faces.

Two 1280 × 720 full-scene renders were inspected with back-face culling:

1. Elevated site view: eye (-180, 290, -90), target (-370, 5, -330), including
   the curved colonnade, Garden road and stadium.
2. Closer three-quarter view: eye (-170, 145, -635), target (-370, 17, -435).

Shader compilation and GL error/state checks passed on Intel UHD Graphics.
Depth testing/writes remained enabled; no blend or polygon-offset state leaked.
No FPS comparison or benchmark was performed for this static massing stage.

Key bindings, teleports, other landmarks and their optimized renderers remained
unchanged. Placement and massing were subsequently approved by the user.

## Part 2 history — silver exterior, superseded by Part 2.5

The original plain wall triangles between Y=0.28 and Y=27.4 were replaced
with a connected, recessed facade skin. The approved foundation, layered
upper rim, oval roof shoulder and rectangular roof placeholder retain their
exact Part 1 triangle positions, normals, UVs and material colors.

- **14 aluminum cladding courses**, including a low sill and ground-floor
  headband; **12** of the silver courses form the upper facade.
- **Four narrow upper glazed strips**, at Y=9.20..10.10, 14.70..15.50,
  20.30..21.20 and 25.50..26.25. A fifth, taller glazed base occupies
  Y=0.70..4.60. The pane faces are recessed 0.28 m from the approved profile.
- **192 panel divisions per course**, approximately 3.4 m wide at the
  reference elevation: **2,688 procedural aluminum panel fields**. These
  are shader divisions on batched curved strips, not separate meshes or draws.
- Narrow continuous beveled metal trims finish all 19 facade courses.
  Cladding is recessed 0.035 m, with 0.09 m shadow grooves at course joints.
- **16 vertical pilasters**, approximately 0.6–0.8 m wide and 0.075 m proud
  of the original wall profile. They stay beneath the unchanged wider eaves
  and within the approved maximum 196 × 240 m building envelope.
- Opaque blue-gray reflective panes and slim shader mullions complete the
  glazed base. Wider pane groups reserve the +Z front entrance axis without
  introducing finished doors, entrance portals, stairs or plaza paving.

An arena-only shader mode supplies soft silver daylight shading, moderate
specular highlights, subtle cool/warm rectangular panel variation, restrained
seams and opaque sky-reflective glass. Its uniform is zero for all other
objects and restored to zero after the stadium. The existing roof uses its
original ordinary opaque material. No transparency, glass-sort modification,
new renderer, texture asset or per-frame geometry update was introduced.

| Rendering cost | Part 1 | Part 2 | Change |
| --- | ---: | ---: | ---: |
| Vertices | 8,038 | 53,070 | +45,032 |
| Triangles | 11,760 | 56,112 | +44,352 |
| Opaque draw calls | 5 | 6 | +1 |
| Static vertex/index storage | 398,336 B | 2,371,584 B | +1,973,248 B |

Six material batches cover foundation, silver panels, glazing, unchanged roof,
unchanged rectangular roof section and facade trims/pilasters. All generation
and uploads remain constructor-only; rendering allocates no geometry buffers.

## Part 2 verification

- Debug x64 build passed, with no new compiler warnings or errors. Existing
  GLFW LNK4099 missing-PDB warnings remain.
- All generated indices, finite positions, unit normals, nondegenerate
  triangles and outward winding passed initialization checks.
- The main weathering shell is closed: every welded edge has exactly two
  oppositely oriented faces. Applied pilasters seat into this skin.
- Foundation and both roof batches passed an exact before/after comparison
  of triangle vertex positions, normals and UVs, including the upper rim.
- Full-scene 1280 × 720 renders were inspected from the +Z colonnade side
  (eye -370,18,-190; target -370,17,-420) and elevated three-quarter view
  (eye -170,145,-635; target -370,17,-435), with back-face culling enabled.
- Shader compilation, OpenGL error checks and render-state restoration
  passed on Intel UHD Graphics. No performance benchmark was requested.
- All unrelated source/project files and earlier working-tree changes were
  preserved. Changes are limited to AstanaArena.cpp/.h, Scene.cpp/.h,
  the scoped ground.frag addition, and this document.

The main entrance, plaza and detailed roof remain future stages. Stop here
for the user's F5 facade review.

## Part 2.5 — continuous silver stadium shell

The regular 192-division pane grid, contrasting rectangular panel seams,
16 vertical pilasters, and closely spaced cladding courses were removed.
Those details created the commercial-building appearance reported in F5.

The replacement facade has **six broad aluminum courses** (including its
low sill) with 25 mm recessed faces and shallow 55 mm band-joint recesses.
The lower glazed level remains distinctly recessed, 320 mm behind the wall
profile, between Y=0.85 and 3.80. It has no repeated bright mullions.

Four narrow upper slit bands occur at unequal elevations and heights:

| Band | Elevation range | Long dark runs |
| --- | --- | ---: |
| 1 | Y=8.35..8.70 | 4 |
| 2 | Y=13.80..14.25 | 4 |
| 3 | Y=19.70..20.00 | 4 |
| 4 | Y=24.55..24.85 | 3 |

These **15 deliberately staggered apertures** wrap along the oval at varying
lengths, with gradual tapered ends. The recess deepens to 200 mm only within
each run. Coverage is baked into the static vertices and used by the existing
arena-only opaque shader mode; between runs, silver infill closes the slits.
No pane assemblies, transparent surfaces or new sorting routines are used.

Monotone Hermite interpolation through the existing wall-profile anchors
softens the bowl's vertical curvature and eases it into the roof rim. The
profile anchors, center, orientation, maximum envelope and height stay fixed.
All cladding remains inside that envelope. Satin silver-gray shading uses
restrained sky fill and highlights, with less than one percent broad tonal
variation instead of a repeated panel grid.

| Rendering cost | Part 2 | Part 2.5 | Change |
| --- | ---: | ---: | ---: |
| Vertices | 53,070 | 25,185 | -27,885 |
| Triangles | 56,112 | 33,320 | -22,792 (40.6% fewer) |
| Opaque draw calls | 6 | 5 | -1 |
| Static vertex/index bytes | 2,371,584 | 1,205,760 | -1,165,824 |

All geometry is generated and uploaded once at initialization. The deleted
pilaster helper and trim batch are no longer allocated or drawn. No Scene,
camera, road, terrain, shared renderer or other landmark code changed.

### Part 2.5 verification

- Debug x64 built successfully, with no new compiler warnings or errors;
  existing GLFW LNK4099 missing-PDB warnings remain.
- Constructor validation passed for finite vertices, unit normals, valid
  indices, nondegenerate triangles and outward winding.
- The entire shell passed the welded-edge closure check: every edge has two
  oppositely oriented incident triangles, including slit transitions.
- Exact before/after triangle position, normal and UV comparisons passed for
  the foundation, upper roof rim, oval roof and rectangular roof placeholder.
  Total bounds remain X=-470..-270, Z=-559..-311 including the foundation,
  and maximum roof Y=39.74635696.
- One front elevation from the colonnade side and one elevated three-quarter
  render were inspected at 1280 × 720, using the same cameras as Part 2.
- Full-scene shader compilation, OpenGL error and state-restoration checks
  passed with back-face culling, depth testing and depth writes enabled.
- Original reference photographs were unavailable locally; no exact image
  comparison is claimed. The supplied smooth-shell description guided this
  revision. No benchmark or FPS comparison was performed.

Changed files: AstanaArena.cpp, AstanaArena.h, the arena-only ground.frag
material block, and this document. All unrelated working-tree changes remain
preserved. Stop for the user's F5 visual review; no commit or push.

## Part 3 — roof form, upper rim and retractable zone

The old radial blend into a sharp rectangular cap produced pinched corner
transitions and a flat-lid appearance. That entire upper mesh was replaced;
the approved Part 2.5 cladding, slit runs, ground-level glazing and foundation
retain exactly the same triangle positions, normals, UVs and materials.

The new roof consists of:

- A smoothly rolled upper fascia seated on the existing Y=27.4 facade edge.
  Its widest ring remains exactly **196 m X × 240 m Z**. The eave varies from
  Y=31.2 at the short ends to Y=32.6 along the central long elevations.
- Broad curved silver roof shoulders rising into a shallow longitudinal crown.
  The roof's maximum remains **Y=39.74635696**, at the crown's central sides;
  its short ends descend gently. Rounded rectangular transitions replace the
  previous corner pinches. The stadium center and orientation are unchanged.
- A **100 × 152 m stepped metallic surround** around the approved **88 × 140 m
  retractable zone**. Its raised rail-like edges are integral mesh surfaces,
  without an overlapping roof or floating frame.
- Two static, partly retracted end cassettes occupy about 24 m at each short
  end of that zone. Narrow longitudinal guide ledges connect them. The clear
  central aperture is approximately **85 × 92 m**, with 3 m rounded corners.
- A recessed architectural well replaces the old roof-level cap. A neutral
  opaque backing at **Y=16** is approximately 23.75 m below the crown. This
  closes the static shell without pretending to model seating or a complete
  stadium interior. It is a deliberate limitation of this roof-only stage.

All surfaces are opaque. Roof silver and the slightly darker cassette/frame
finish reuse the existing arena material; the approved facade shader is
unchanged. No shader edits, new renderer, transparency pass, animation,
per-frame geometry generation or dynamic buffer uploads were introduced.

### Reference handling

The user's reference images were unavailable locally. The architect's own
[Astana Arena project description](https://www.tabanlioglu.com/project/astana-arena/)
was consulted for its elliptical stadium and retractable-roof concept. Its
photographs could not be inspected through the available image links, so
exact photographic matching and mechanical accuracy are not claimed. All
geometry is original procedural code, with no imported model or copied mesh.

### Part 3 cost and validation

| Rendering cost | Part 2.5 | Part 3 | Change |
| --- | ---: | ---: | ---: |
| Vertices | 25,185 | 32,045 | +6,860 |
| Triangles | 33,320 | 45,472 | +12,152 |
| Opaque draw calls | 5 | 6 | +1 |
| Static vertex/index bytes | 1,205,760 | 1,571,104 | +365,344 |

The six batches contain 980 foundation, 16,072 facade cladding, 7,056 opaque
glazing, 13,328 upper fascia/roof, 3,332 recessed well and 4,704 guide/cassette
triangles. Everything is generated once at scene initialization.

- Debug x64 build passed with no new compiler warnings or errors. Existing
  GLFW LNK4099 missing-PDB warnings remain.
- Initialization verified finite positions, unit normals, valid indices,
  nondegenerate triangles and outward winding.
- The entire connected shell, including the aperture and recessed backing,
  passed welded-edge closure: two opposite incident faces per edge.
- Exact before/after triangle comparisons passed for the complete foundation
  and both facade batches. The maximum height and footprint bounds are unchanged.
- Front, elevated three-quarter and near-top-down 1280 × 720 renders were
  inspected with back-face culling, depth testing and depth writes enabled.
  The first two camera positions match Part 2.5; the top view uses eye
  (-370,330,-434), target (-370,0,-435).
- Full-scene shader compilation, OpenGL errors and material/state restoration
  passed on Intel UHD Graphics. No performance benchmarks were run.
- Only AstanaArena.cpp/.h and this document changed. All other source,
  shaders, roads, landmarks, controls and existing working-tree changes remain
  preserved. No commit or push; stop for the user's F5 roof review.

## Part 3.5 — cassette panels, guide rails and structural lining

The two approved cassettes were still smooth and undivided, while the deep
well had one dark material and no structural articulation. Those finishes
caused the reported featureless-pit appearance. The existing cassette,
aperture, recess and outer-roof mesh positions were preserved; static metal
details and scoped material finishing now make the mechanism readable.

### Roof details

- **40 longitudinal standing seams**, 20 on each existing end cassette,
  approximately 160 mm wide and 100 mm high, defining about **42 main panel
  bays**. The 20 m-long seams run along Z, between |Z|=48 and 68.
- **Six stronger cassette ribs**, three per leaf, at local X=-28, 0 and 28.
  Their 340 mm-wide profiles rise 230 mm above the underlying panels.
- **Two 136 m-long I-profile guide rails** at local X=±43.25, Z=-68..68.
  Joined flanges and webs follow the actual existing roof triangles. Rail
  flanges are 540/440 mm wide; the top is 420 mm above its supporting ledge.
- **Two end headers**, at local Z=±47.2, connect the cassette edges to both
  guide rails. **12 restrained bearing seats** reinforce the protected ledges.
- Subtle grouped aluminum tones and antialiased Z-aligned panel seams are
  limited to the cassette surfaces. They accompany real seam geometry and
  remain readable when those thin members become subpixel in elevated views.

All these details are seated using initialization-only barycentric height
queries against the approved frame triangles, including its rounded corners.
The highest new detail is **Y=39.091354**, below the unchanged crown maximum
**Y=39.74635696**. No floating approximated rail centerline is used.

### Opening finishing

- The inner wall now has a restrained silver-gray metallic lining, narrow
  horizontal joints and depth-dependent tonal shading, using existing lighting.
- **Two continuous horizontal lining girders**, at Y=24 and 30, and **20
  spaced vertical stiffeners** replace the featureless interior wall treatment.
- The **85 × 92 m roof-level aperture** remains clear. Below the coping,
  lining ribs/belts project only **0.11–0.14 m** into the well; this is a local
  lining detail, with no change to the upper opening or stadium dimensions.
- The existing backing at Y=16 remains a simple neutral opaque surface with
  a softer gray tone. No field, seats, interior rooms or roof animation were added.

### Cost and validation

| Rendering cost | Part 3 | Part 3.5 | Change |
| --- | ---: | ---: | ---: |
| Vertices | 32,045 | 48,141 | +16,096 |
| Triangles | 45,472 | 55,088 | +9,616 |
| Opaque draw calls | 6 | 7 | +1 |
| Static vertex/index bytes | 1,571,104 | 2,201,568 | +630,464 |

The entire new steel assembly uses one material batch. Its 90 individual
fabricated solids include the separate rail/header flange and web profiles;
they are not individual draw calls. All construction, surface queries and
buffer uploads run once. No glass sorting, vehicle LOD/instancing, Scene
integration, camera or unrelated renderer changes were made.

- Debug x64 build passed with no new compiler warnings/errors; existing GLFW
  missing-PDB linker warnings remain when the dependency is linked.
- All six original stadium batches passed exact triangle position/normal/UV
  comparisons, including the complete outer roof, cassettes and opening.
- The original connected stadium shell remains closed. All 90 new metal
  solids passed separate welded-edge closure and opposite-edge winding checks.
  Flange/support contacts are intentional assembled structural joints.
- Constructor checks passed for finite vertices, unit normals, valid indices,
  nondegenerate triangles, outward winding and the approved height limit.
- One elevated three-quarter and one top-down 1280 × 720 view were inspected.
  Full-scene shader compilation, OpenGL error and state-restoration checks
  passed on Intel UHD Graphics, with culling and depth testing/writes enabled.
- No performance benchmarks or FPS claims were made for this static detail stage.

Changed files: AstanaArena.cpp/.h, the arena-only material code in ground.frag,
and this document. Other working-tree changes remain intact. Stop for the
user's F5 review; no commit or push.

## Orientation correction — complete rigid quarter-turn

Only the shared model transform changed from 0 to **+90 degrees about Y**.
The center remains (-370,0,-435) and scale remains (1,1,1). Every one of the
seven material batches uses this transform, so shell, facade, roof, cassettes,
lining, rails and foundation rotate together. This is a proper rotation with
determinant +1, without reflection, deformation or regenerated mesh geometry.
Local +Z now points toward world +X; the Garden-facing broad side is world +Z.

The local vertex/index buffers and material ranges are byte-for-byte identical
to Part 3.5. Mesh closure, normals and winding are preserved by the rigid
transform; constructor validity checks and full-scene back-face culling passed.
The maximum height remains 39.74635696 m, and rendering cost stays at 55,088
triangles / 7 opaque draw calls, without extra per-frame work.

Debug x64 built successfully. One elevated view from (-170,190,-180) toward
(-370,15,-400), and one pedestrian view from (-370,3,-278) toward
(-370,18,-435), were inspected from the Botanical Garden side. Both full-scene
renders passed shader/OpenGL error and state-restoration checks. The corrected
footprint fits the existing terrain, with 64 m sidewalk clearance and no
changes to roads, paths, colonnade, vegetation or other landmarks.

Changed files for this correction: **src/AstanaArena.cpp** and this document.
All previous working-tree changes were preserved. Stop for F5 review;
no commit or push.

## Part 4 — main entrance, stairs, ramps and signage

The entrance is derived from the stadium's rotated coordinates: local -X
is outward toward world +Z, while local +Z runs across the entrance toward
world +X. The shared +90-degree model transform remains unchanged. Entrance
geometry uses `(local x,y,z)=(-depth,y,side)`, giving world position
`(-370+side,y,-435+depth)`.

The central recessed door field is approximately **(-370,1.6,-347.1553)**,
facing the Garden crossing at X=-370, Z=-266..-254 and the curved colonnade.
It is on the approved long side, without relocating or rotating the stadium.

### Entrance architecture

- A **48 m-wide curved glazed entrance field**, with **six 5 m-wide framed
  double-door groups** and side glazing. Thresholds are Y=1.60/1.62, door
  transoms Y=5.20 and the glazed head Y=5.80. Thin silver framing, central
  mullions and dark door handles give the groups architectural depth.
- The obsolete cladding/glazing inside local Z=-24..24, Y=0.28..6.50 on
  local -X was removed by initialization-only triangle clipping. The new
  door field therefore has no original opaque facade directly behind it.
  Stone sill panels and metallic side/head reveals finish the cutout.
- A **52 m-wide curved silver entrance canopy**, Y=6.00..6.35, projects
  3 m beyond the original wall. Its rear extends over the recessed head
  reveal for a physical structural connection, rather than floating in front.
- **ASTANA ARENA** lettering is centered above the entrance at Y=7..9,
  approximately 23.45 m wide and 2 m high. Original geometric glyphs are
  shallow 160 mm extrusions, mounted into a curved silver fascia. Letters
  are dark blue-gray and share one batch with the small dark door details;
  there are no fonts, alpha textures or per-letter draw calls.

### Stairs and accessible approaches

- A **56 m-wide level upper landing**, Y=1.60, joins the curved thresholds
  to local outward depth=103 (world Z=-332). It is supported from terrain
  and integrates above the unchanged original foundation.
- A **48 m-wide ceremonial staircase**, **4.5 m total run**, connects
  Y=1.60 to the Y=0.04 lower approach. It has **10 × 156 mm risers** and
  **450 mm treads**, at world X=-394..-346, Z=-332..-327.5.
- The lower arrival landing is **52 × 5 m**, ending at world Z=-322.5.
- Two mirrored **32 × 3.2 m ramps** flank the landing. Each rises 1.56 m
  from Y=0.04 to 1.60: **4.875% / approximately 1:20.5**. Outer foot landings
  and slim 0.98 m handrails finish the routes. Ramps run along world X,
  rather than pointing into the stadium or crossing the road.
- Light-gray stone uses a batched procedural 2.4 × 1.2 m staggered paving
  pattern. Stone surfaces and all architectural details are opaque/static.

New entrance construction bounds are approximately X=-434..-306,
Z=-350.002..-322.5, Y=-0.08..9.25. The body and original foundation bounds
remain unchanged; the entrance steps extend locally beyond the foundation
as intended. The arrival area remains **51.5 m from the existing Garden
sidewalk** and **56.5 m from the road edge**. An open central approach axis
to the crossing remains reserved for the future plaza. No complete plaza,
new road, parking, trees or other landscaping was added.

### Cost and verification

| Rendering cost | Before Part 4 | Part 4 | Net change |
| --- | ---: | ---: | ---: |
| Vertices | 48,141 | 55,356 | +7,215 |
| Triangles | 55,088 | 58,113 | +3,025 |
| Opaque draw calls | 7 | 11 | +4 |
| Static vertex/index bytes | 2,201,568 | 2,468,748 | +267,180 |

The four additional batches are stone (424 triangles), entrance metalwork
(2,172), glazing (96), and lettering/handles (620). Local facade clipping
removes 287 net old triangles, producing the reported net total. All clipping,
letter triangulation, geometry and buffer uploads run once at initialization.
No transparency pass, sorting/cache changes or per-frame allocations were added.

- Debug x64 build passed without new compiler warnings/errors. Existing GLFW
  missing-PDB warnings remain when the dependency is linked.
- Initialization checks passed for finite vertices, unit normals, index bounds,
  nondegenerate triangles and outward winding. Full-scene culling, shader,
  OpenGL error and render-state checks passed in both inspected views.
- Exact triangle position/normal/UV comparisons preserved the foundation,
  full roof, cassettes, aperture and all roof mechanism geometry. Original
  facade triangles outside the local cutout are unchanged.
- A centroid/clip-region check confirmed no residual old facade inside the
  entrance patch. Door thresholds, landing heights and ramp slopes align.
- Frontal pedestrian and elevated three-quarter renders were inspected at
  1280 × 720. The first camera is (-370,3,-288), target (-370,5,-345); the
  second is (-220,115,-235), target (-370,12,-390).
- Stadium center, +90-degree rotation, 240 × 196 m body and 39.74635696 m
  maximum height remain unchanged. Roads, colonnade, Garden, other landmarks,
  camera controls and existing optimizations were preserved.

Changed files: AstanaArena.cpp/.h, the scoped entrance-stone material in
ground.frag, and this document. No performance benchmark or FPS claim;
stop for F5 visual review. No commit or push.

## Part 5A — ground-level football pitch and two-tier bowl

### Removal of the temporary interior obstruction

The old opaque backing was at **Y=16**, inside an **85 × 92 m vertical shaft**.
Removing the backing alone would leave the shaft cutting across the ends of
the 105 m pitch and blocking views into a wider stadium bowl. The backing
and lower shaft were therefore removed. The lined roof collar now terminates
at **Y=33**, below its unchanged coping.

The two former shaft belts at Y=24/30 are now seated within that collar at
**Y=33.6/35.8**; the 20 lining stiffeners are trimmed to start at Y=33.05.
This is a local interior-liner adaptation. The original exterior, roof top,
cassettes, guide rails, standing seams, headers and bearing seats are unchanged.

### Pitch and goals

- Playing dimensions: **105 × 68 m**, at **Y=0.60**, centered at
  **(-370,0.60,-435)**. Local Z remains longitudinal, becoming world X under
  the approved +90-degree stadium rotation. World playing bounds are
  X=-422.5..-317.5, Z=-469..-401.
- A rounded **80 × 122 m grass apron** includes 6 m safety margins beside
  the touchlines and 8.5 m behind the goal lines. A separate approximately
  3 m-wide circulation ring joins the bowl toe.
- Twelve 8.75 m mowing stripes and restrained grass variation use an opaque
  arena-only material. All pitch markings are analytically shaded on the
  same surface, avoiding overlapping paint meshes or depth offsets.
- Markings include the boundary, halfway line, center circle/spot, both
  penalty and goal areas, penalty spots/arcs and all four corner arcs.
- Proportions follow [IFAB Law 1](https://www.theifab.com/laws/latest/the-field-of-play/):
  120 mm lines, 9.15 m circle/arc outside radii, 16.5 m penalty-area depth,
  5.5 m goal-area depth, 11 m penalty marks and 1 m corner arcs. Area widths
  are 40.32 m and 18.32 m. Lines are kept inside the 105 × 68 m boundary.
- **Two goals** have **7.32 m clear width and 2.44 m clear height**. Eight-sided
  100 mm front posts/crossbars, supported rear frames and 2 m-deep open cord
  grids form static opaque models. Both goals and nets share one draw call.

### Spectator-bowl foundation

The bowl blends a rounded rectangular toe into an elliptical outer contour,
using the full stadium volume instead of the old roof-sized shaft.

- Inner toe: **86 × 128 m**, retaining edge at Y=1.35.
- Lower structural bank rises to **Y=10.2**; a roughly **5 m intermediate
  concourse** separates the tiers.
- Upper bank reaches **Y=21.4**, followed by an approximately 3 m rear walk
  and a **Y=22.3 parapet**.
- Overall bowl foundation: **170 m wide × 216 m long** (world Z × X), with
  outer world bounds X=-478..-262 and Z=-520..-350.
- Closed rear structural walls and an underside support the banks. Bare
  precast surfaces have restrained course joints; no individual seats,
  spectators, scoreboards or advertising are modeled.
- Upper inward cladding and a **220 mm inward-offset roof soffit** prevent
  the city from showing through the old one-sided exterior roof when viewed
  from inside. These interior surfaces reuse the bowl batch and do not alter
  the approved exterior roof positions, normals or materials.

### Visibility and validation

The roof-level aperture remains **85 × 92 m**. A perspective top-down view
from **(-370,215,-434.9)** toward **(-370,0.60,-435)** shows the complete pitch
through the opening. The elevated interior view uses **(-370,30,-515)**,
looking at the same target. Both show all four field corners and both goals.
The fixed partially retracted cassettes can naturally conceal field ends from
some other exterior angles; neither the field nor the roof was rescaled to
remove that expected perspective occlusion.

- Debug x64 build passed without new compiler warnings/errors. Existing GLFW
  missing-PDB linker warnings remain when that dependency is linked.
- Constructor checks passed for finite vertices, unit normals, valid indices,
  nondegenerate triangles and outward winding. Bowl envelope checks confirm
  the banks fit inside the facade and below the roof.
- Exact triangle/normal/UV comparisons preserved the foundation, facade,
  exterior roof, cassette panels and every entrance batch. The first 6,240
  mechanism triangles (seams, ribs, guide rails, headers and bearings) also
  remain exact; only the obsolete deep-shaft finish was adapted above.
- Mesh checks confirm no Y=16 backing remains, all pitch vertices lie at
  Y=0.60, and the stadium maximum remains Y=39.74635696.
- Conservative two-sided ray checks against actual roof, collar, mechanism,
  soffit and bowl triangles found no obstruction to the four pitch corners,
  center or penalty spots from either inspection camera.
- Both 1280 × 720 full-scene renders passed shader compilation, OpenGL error
  and state-restoration checks, including back-face culling and depth writes.

### Rendering cost

| Rendering cost | Part 4 | Part 5A | Net change |
| --- | ---: | ---: | ---: |
| Vertices | 55,356 | 79,404 | +24,048 |
| Triangles | 58,113 | 91,017 | +32,904 |
| Opaque draw calls | 11 | 14 | +3 |
| Static vertex/index bytes | 2,468,748 | 3,633,132 | +1,164,384 |

The new batches contain 196 pitch/apron, 31,752 bowl/interior soffit and 3,112
goal/net triangles. Removing the old backing and shortening the collar saves
2,156 old triangles. There are no per-marking or per-net draw calls.
Generation, roof-height queries and buffer uploads occur only at initialization;
there are no new transparency passes, allocations or geometry uploads per frame.
MEGA/EXPO glass caches and vehicle instancing/LOD remain unchanged.

Changed files: AstanaArena.cpp/.h, arena-only pitch/bowl/collar material code
in ground.frag, and this document. All unrelated working-tree changes remain
intact. No performance benchmark or FPS claim; stop for F5 visual review.
No commit or push.

## Part 5B — detailed blue/white two-tier grandstands

### Seating layout

- **27,644 seats total:** **10,672 lower-tier** and **16,972 upper-tier**.
- **16 lower rows** and **20 upper rows**, arranged in **16 blocks per tier**.
  The existing 5 m intermediate circulation band and upper rear concourse
  remain continuous. Seats face inward around all four sides and corners.
- **25,232 blue seats and 2,412 white seats** (approximately 8.7% white).
  White row bands and controlled chevrons create coherent sections without
  random checkerboard coloring. Deterministic blue tones add restrained depth.
- Chairs measure approximately **0.47 m wide × 0.51 m deep × 0.936 m high**.
  Pans sit about 0.42 m above level row terraces; nearby models have beveled
  pans/backs and a compact support pedestal. Centers are spaced **0.60 m**.

The old visible sloped bank faces were replaced by supported horizontal row
terraces and risers, rather than covering a sloped plane with floating chairs.
The internal contour mapping was also corrected: the rounded inner contour
and elliptical outer contour now use the same polar ray. Their dimensions,
footprints and tier heights stay fixed, while the compressed corner gaps from
the old mismatched parameterization are removed. The lower tier uses 16 rows
to retain full chair size and avoid overlapping corner rows.

### Access and railings

- **16 aligned radial aisles per tier**, nominally 2 m wide, divide the seating.
  The verified nearest seat footprint stays at least **1.0649 m from the aisle
  centerline**. No seat intrudes on the stair routes.
- Each row rise is split into **three aisle steps**: 48 lower and 60 upper
  steps per aisle. Four additional lead-in steps connect every lower aisle to
  the existing technical circulation apron without touching the pitch turf.
- Pale concrete treads/risers provide visual contrast. Slim metallic guards
  finish the tier fronts, leave aisle mouths open and provide handrails along
  selected stair routes. No new transparent materials are used.

### Instancing and LOD

`AstanaArenaSeats.cpp/.h` owns a small local opaque renderer. It does not edit
the shared city shader, glass caches or parked-vehicle renderer. Every chair
uses one of **three shared indexed meshes**:

| Chair LOD | Triangles per mesh | Detail |
| --- | ---: | --- |
| LOD0 | 52 | Beveled pan/back and pedestal |
| LOD1 | 24 | Closed pan/back, simplified support detail |
| LOD2 | 8 | Double-ply pan/back shells, omitted subpixel rims/legs |

Initial distance thresholds are **20 / 70 m**, using conservative row-block
bounds. Hysteresis uses **16/24 m** and **60/80 m** transition limits. All
levels retain the chair footprint and pan/back silhouette. There is no fade,
transparency pass or per-seat draw call.

The 576 row-blocks are classified only when camera position changes. Instance
IDs are compared against cached groups; preallocated buffers are uploaded only
when a group actually changes. Camera rotation alone does not change distance
LOD. Stationary repeated views produce no further uploads. A stadium-level
frustum test skips all seating draws when the district is outside the view.
All mesh construction and buffer allocation happen once during initialization.

### Rendering cost

- The unchanged stadium/field/entrance batches now contain **81,609 triangles**
  after removing **9,408 old sloped-bank triangles** from the interior batch.
- New static geometry contains **30,336 terrace/stair triangles** and **15,072
  railing triangles**, batched in **two draws**.
- The three shared chair meshes add only **84 unique triangles**. Total unique
  loaded stadium geometry is **127,101 triangles**, versus 91,017 before seating.
- Seats add at most **three instanced draws**, giving **five extra draws total**
  and **19 stadium draws** when all LODs are populated.

Actual submitted counts in the two inspected views include instances:

| View | LOD0 / LOD1 / LOD2 seats | Seating renderer triangles, including terraces/rails | Entire stadium triangles |
| --- | --- | ---: | ---: |
| Interior overview | 1,806 / 8,234 / 17,604 | 477,768 | 559,377 |
| Close spectator view | 2,745 / 7,295 / 17,604 | 504,060 | 585,669 |

These are measured geometry submissions, not FPS measurements. Visible F5
performance remains for user review; no lengthy benchmark suite was run.

### Validation and preservation

- Debug x64 build passed without new compiler warnings/errors. Existing GLFW
  missing-PDB warnings remain when that dependency is linked.
- Static geometry and every LOD passed initialization checks for finite
  positions, unit normals, valid indices, nondegenerate triangles and winding.
- All 27,644 chair footprints passed spatial separation/SAT checks, including
  adjacent rows and curved corners. Minimum within-row center spacing measured
  0.5997 m. Feet match the corresponding terrace levels; all seats face inward
  and remain clear of the pitch, goals and roof.
- Repeating either stationary camera view kept upload counters unchanged.
  The first view populated three groups; teleporting to the close view updated
  only two groups, with the distant group reused unchanged.
- Full-scene OpenGL and shader checks passed with back-face culling and depth
  testing/writes. The local renderer restores program, VAO, array buffer,
  blending and depth state before the remaining city renders.
- The overview uses eye (-370,30,-515), target (-370,0.60,-435). The close view
  uses eye (-367,11.9,-498.5), target (-367,12,-503), on the intermediate
  spectator circulation level. Both 1280 × 720 renders were inspected.
- Exact triangle position/normal/UV comparisons preserve every stadium batch
  except the deliberately refined internal bowl. Pitch, goals, exterior,
  entrance, roof and mechanism remain unchanged. Stadium position, rotation,
  footprint and maximum height are preserved.

Changed files: AstanaArena.cpp/.h, new AstanaArenaSeats.cpp/.h, one Scene.cpp
render call, both Visual Studio project files, and this document. Shared shaders
and all unrelated working-tree changes remain untouched. Far LODs intentionally
omit fine supports/edge details; no spectators or animated seating are included.
Stop for F5 visual/performance review. No commit or push.

## Part 6A — Main entrance plaza and pedestrian connections

The Garden-facing entrance remains unchanged. Its lower landing ends at
world Z=-322.5; the existing southern Garden sidewalk begins at Z=-271.
The new forecourt fills this 51.5 m gap, with a maximum width of 132 m
(X=-436..-304). Chamfered shoulders and a formal taper meet the existing
8 m crossing alignment at X=-374..-366. The original sidewalk and crossing
are retained, including the 5 m sidewalk between the plaza and carriageway.

- Main plaza paving area: 5,255 square metres. Two secondary ramp links add
  74.4 square metres, giving approximately 5,329.4 square metres in total.
- Two mirrored side connections join the original ramp footpads at
  (-432,0.04,-331.8) and (-308,0.04,-331.8), then meet the forecourt at
  (-426,0.04,-322.5) and (-314,0.04,-322.5). Their gentle curves have 4 m-wide
  thresholds and approximately 2.9 m minimum perpendicular walking width.
- The central 24 m crowd approach is framed by flush tonal bands, narrowing
  only at the road-side taper to match the approved 8 m crossing.
- Paving top matches the lower landing at Y=0.04, then gently rises 20 mm
  over the final 8 m to meet the existing sidewalk at Y=0.06. Embedded stone
  edge faces finish at Y=-0.02. No terrain expansion or raised platform.
- Staggered 2.4 x 1.2 m procedural stone tiles use restrained warm-gray
  variation, antialiased joints and flush accent bands. The new shader mode
  applies only to this plaza; existing material branches are unchanged.
- Grass pockets behind the outer forecourt and along its chamfered edges
  remain available for future landscaping and furniture. No vegetation,
  lights, benches, parking or new roads were added.

### Rendering and validation

Added 388 vertices and 194 triangles in one static opaque material batch,
approximately 14.4 KiB of vertex/index storage. The Arena main mesh now has
81,803 triangles in 15 draws; seat renderer costs and LOD behavior are unchanged.
There are no per-frame mesh updates or new transparency passes. No FPS benchmark
was run for this small addition; visible performance remains for F5 review.

Debug x64 built successfully. Existing GLFW missing-PDB linker warnings remain.
Initialization validated finite vertices, unit normals, valid indices and
nondegenerate, correctly wound triangles. Paving overlap checks passed against
the approved entrance paving, existing sidewalk and road, and against itself.
The preexisting 14 Arena geometry batches are byte-identical to Part 5B.

Two full-scene 1280 x 720 renders were inspected: elevated eye (-185,160,-145)
toward (-370,2,-335), and pedestrian eye (-370,2,-273) toward (-370,5,-346).
Shaders compiled and OpenGL checks passed, including restored depth, blend and
material states. This hidden validation context used Intel UHD Graphics; it is
not a measurement of the user's visible F5 GPU or frame rate.

Changed for Part 6A: src/AstanaArena.cpp, src/AstanaArena.h,
shaders/ground.frag and this document. Scene integration, terrain, seats,
vehicles, glass caches, roads, Garden and unrelated working-tree files are
unchanged. Stop for F5 visual review. No commit or push.

## Part 6B — West-side parking and access

### Site and capacity

The selected empty parcel is west of the stadium, outside the EXPO ring and
away from the tighter Khan Shatyr side. Parking and perimeter walks occupy
X=-614..-542, Z=-522..-396.4: 72 x 125.6 m, centered at (-578,0,-459.2).
The eastern perimeter walk remains 48 m west of the nearest stadium foundation
edge, except for the intended connecting pedestrian path. No terrain extension,
building relocation, vehicles, vegetation or furniture is included.

- 212 spaces total: 208 standard and four accessible.
- Standard bays are 5 x 2.6 m. Accessible bays are 5 x 3.6 m with individual
  1.6 m transfer strips, blue paving and white accessibility symbols.
- Six parking rows form three paired modules. Seven-metre aisles connect
  through two 8 m head aisles and the eastern distributor lane.
- Two 4 m transverse walking routes interrupt the rows and continue across
  marked aisle crossings. Four-metre perimeter walks surround the lot.
- Six 3 x 31.2 m grass divider reservations have low concrete edging;
  their existing terrain remains exposed for future planting.

### Road and pedestrian connections

The public T-junction is centered on (-460,0.04,-260), on the existing southern
Garden perimeter street. Its asphalt joins the unchanged street edge at
Z=-266, spanning X=-469.5..-450.5. The only Garden changes are a local sidewalk
and curb opening within X=-474..-446 and removal of five conflicting center
dashes. The existing public crossing at X=-370 is unchanged.

The private road is 7 m wide with 3 m sidewalks, 6 m junction corner radii and
two 14 m centerline-radius bends. It passes (-460,-290), (-474,-304),
(-547.5,-304) and (-561.5,-318), then reaches the parking header at
(-561.5,0.04,-400.4). Length from the public asphalt edge to that header is
approximately 223.88 m. The road does not cross the approved entrance plaza.

A 4 m driveway zebra with flush side ramps preserves Garden sidewalk
continuity at Z=-282..-278. Another zebra crosses the parking entry at
Z=-400.4..-396.4. Internal crossings connect both walking strips and the
accessible transfer route to the eastern sidewalk.

The eastern sidewalk continues north and meets the original west entrance
ramp footpad at X=-434, Z=-336.2..-332.2. A shorter, 4 m-wide side approach
continues the southern transverse route to the existing western stadium
foundation, at Z=-443.6..-439.6. Its last approximately 12 m rises gently from
Y=0.06 to Y=0.28 and follows the actual polygonal foundation edge. No new side
door or change to the approved building is introduced.

### Geometry, rendering and checks

Four new static opaque batches reuse the existing Arena buffers and shaders:

| Batch | Triangles |
| --- | ---: |
| Asphalt and access road | 206 |
| Stone walks, ramps and curbs | 1,442 |
| White bay markings, symbols, crossings and arrows | 2,172 |
| Blue accessible-bay surfaces | 8 |
| Parking additions total | 3,828 |

The local Garden rectangle splits add 24 triangles and remove 10 paint
triangles, yielding **3,842 additional scene triangles and four draw calls**.
The Arena main mesh now has 85,631 triangles in 19 draws; the seat renderer is
unchanged. Added Arena vertex/index storage is 290,928 bytes. Generation occurs
once during construction; there are no new transparency passes, per-frame
allocations, GPU buffer rebuilds or shader edits.

Debug x64 build passed with no new compiler warnings/errors. Preexisting GLFW
missing-PDB linker warnings remain. Initialization validated all 212 bay bounds
and separation from the two transverse walking routes, plus mesh indices,
finite/unit normals, nondegenerate triangles and winding. Geometry checks found
no overlapping base paving or intersections with approved plaza, entrance
paving or foundation. All preexisting 15 Arena material batches are
byte-identical to Part 6A, including the approved plaza. Seats, glass caches,
vehicle instancing, controls and unrelated landmark files are unchanged.

The elevated overview (eye -720,190,-210; target -490,2,-390) and parking-level
view (eye -611,2,-395; target -572,3,-468) were inspected at 1280 x 720. Full-scene
shader and OpenGL state/error checks passed. This hidden context used Intel UHD
Graphics; no new FPS measurement or performance benchmark is claimed.

Changed for Part 6B: src/AstanaArena.cpp/.h, new src/AstanaArenaParking.h,
src/BotanicalGarden.cpp, the Arena render-call arguments in src/Scene.cpp,
both Visual Studio project files, and this document. Planting, cars and
furniture remain for future stages. Stop for F5 visual review. No commit/push.

## Part 6C — Plaza landscaping and urban furniture

All additions are initialization-only geometry appended after the 19 approved
Arena batches. No building, road, parking marking or approved paving is changed.

| Addition | Count and distribution |
| --- | --- |
| Parking trees | 24: three in each of six divider reservations, plus six outside the perimeter walks |
| Plaza trees | 12 in six mirrored pairs about X=-370, on grass outside the paving |
| Deciduous / slender trees | 18 rounded and 18 slender ornamental specimens overall |
| Low shrubs | 96: 72 in parking dividers, 24 beside the flower gardens |
| Ornamental grass clusters | 72: 36 in dividers, 18 near perimeter trees, 18 near gardens |
| Decorative flowerbeds | Six mirrored elliptical beds, each 5 x 6 m, with low stone borders |
| Flower rosettes | 384: 96 each in red, white, yellow and purple, in organized curved rows |
| LED-style poles | 24: 12 around parking/approaches and 12 framing the plaza |
| Benches | 12: eight along plaza sides, four on new pads beside the eastern parking walk |
| Waste bins | 10: six near plaza benches and four at the parking rest pads |

The 18 divider trees have crown bounds no wider than 2.32 m, fitting inside
the existing grass reservations without covering bays or aisles. Six broader
parking specimens stand at X=-619 and -537, outside the approved walks.
Plaza tree pairs are at side offsets 74, 74, 73, 50, 22 and 35 m, and respective
Z positions -318, -307, -295, -284, -276 and -327. The four forward trees were
set farther outward so even their conservative crown bounds clear the taper.
The central crowd approach, stairs, ramps and signage sightline remain open.

Flowerbed centers are (-370 +/- 74, -312), (-370 +/- 73, -289) and
(-370 +/- 37, -279), on unchanged Y=0 grass. Stone rims rise only 0.16 m;
soil is at Y=0.08. Existing Tree crown offsets are reused, with efficient
smooth indexed ellipsoids and narrow ornamental proportions. Bench and
StreetLight component transforms are baked from the existing generators.
The 6.8 x 7.5 m rest pads touch the east walk at X=-542 and are centered at
Z=-490, -455, -406 and -365; furniture feet match the paving at Y=0.06.
Plaza furniture stands on the original Y=0.04 paving. Parking light poles are
about 7.6 m high; pedestrian poles about 5.9-6.1 m. Diffusers are opaque,
unlit material surfaces, with no dynamic light sources.

### Rendering and validation

Added **34,656 triangles, 48,864 vertices and 11 material draw calls**.
Additional vertex/index storage is 1,979,520 bytes (about 1.89 MiB).
The Arena main mesh now has 120,287 triangles in 30 static draws; the existing
seat renderer is unchanged. Folded grass/petals have facet-specific normals,
while tree and shrub ellipsoids use smooth scale-correct normals. There are
no per-object draws, per-frame generation, allocations, uploads, additional
transparency passes or shader changes. Existing glass caches, vehicle LOD,
seat instancing and FPS/navigation controls remain untouched.

Debug x64 built successfully with no new compiler warnings/errors. Existing
GLFW missing-PDB linker warnings remain. Initialization checked finite/unit
normals, valid indices, triangle winding and nondegenerate faces. Placement
checks verified every registered planting footprint, including full crown
bounds, against the original paving, curbs and roads. Benches/bins are fully
supported by paving, and tree crowns are separated from lights/furniture.
All 19 preexisting Arena batches are byte-identical to Part 6B, preserving
the approved plaza, 212 parking spaces, building, roof, entrance and pitch.

The final full-scene overview (eye -720,190,-210; target -490,2,-390) and
pedestrian entrance view (eye -370,2,-273; target -370,5,-346) were inspected
at 1280 x 720. Shader compilation, OpenGL errors, depth/blend state and
material-flag restoration passed. The hidden validation context used Intel
UHD Graphics; no claim of a newly measured visible F5 frame rate is made.
No benchmark suite was run. Visual/performance approval remains for F5.

Changed: src/AstanaArena.cpp/.h, new src/AstanaArenaLandscape.h, both Visual
Studio project files and this document. Botanical Garden, Scene integration,
parking/access code and all unrelated working-tree files remain unchanged.
Stop for F5 visual review. No commit or push.
