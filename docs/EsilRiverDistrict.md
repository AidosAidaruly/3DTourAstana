# Esil River and Atyrau Bridge ? Part 1 site plan

This is a proposed layout in the project's stylized city coordinates, not a surveyed geographical reconstruction. All dimensions are meters. Part 1 implemented navigation and continuous ground coverage. Parts 2 through 6A add the recessed river, banks, promenades, bridge structural base, sculptural canopy and open triangular steel lattice described below. The original planning coordinates are retained for context; landscaping and reservation-marker geometry remain unbuilt.

## Coordinate inspection

| Existing landmark / site | World X / Z | Source / checked extent |
| --- | --- | --- |
| Baiterek | 0 / 0 | `Scene::baiterekPosition`; plaza X=-50..50, Z=-30..30 |
| Talan Towers | 85 / -65 | `Scene::talanTowersPosition`; forecourt X=55..115 |
| Ak Orda | 0 / 370 | `Scene::akOrdaPosition`; ceremonial route follows X=0, gardens approximately X=-100..100 |
| Khan Shatyr | 0 / -420 | `Scene::khanShatyrPosition`; outer road R132..140; radial parking to R157 |
| Abu Dhabi Plaza | -125 / 125 | `Scene` instance; access roads centered near X=-200 and -63 |
| Botanical Garden | -370 / -130 | `BotanicalGarden::center`; interior X=-475..-265, Z=-245..-15; perimeter roads remain outside those bounds |
| EXPO | -765 / -130 | `ExpoDistrict::center`; ring road R196..210, outer walk to R215.4 |
| First EXPO residence | -991 / 65 | `ExpoResidence::anchor`, body 72 x 70 |
| Premium residence ensemble | -586 / 132 | `ExpoPremiumResidence::anchor`, plot X=-650..-514, Z=68..204 |
| MEGA Silk Way | -950 / -645 | Body 160 wide x 340 long; parking X=-1173..-1070, Z=-812..-488 |

EXPO and MEGA are west/negative-X of Baiterek. Positive X is the opposite side in this scene. The chosen river corridor lies entirely beyond the former eastern terrain edge X=300, clear of the inspected building, road and landscaping footprints. No existing landmark or road is relocated. Future bank paths must stay outside existing Talan and Ak Orda sites; a connection back to Baiterek is reserved for a later task.

## Proposed district reservation

| Item | Proposed bounds / alignment |
| --- | --- |
| Complete district including both banks | X=330..710, Z=-650..750 |
| River water corridor | X=420..600, Z=-600..700 |
| Water centerline / width | X=510; 180 m wide |
| Flow direction in the model | Along +Z; ends have 50 m transition reserves before the district limits |
| City-side bank reserve | X=330..420, 90 m wide |
| Far-bank reserve | X=600..710, 110 m wide |
| Future promenade corridors | Approximately X=400..408 and X=612..620 (8 m wide), following Z; adjust locally for abutments |
| Future bridge center (ground reference) | (510, 0, 100) |
| Future bridge longitudinal alignment | World X; approximately X=390..630, Z centered at 100 |
| Preliminary deck envelope | 240 m long x 12 m wide; nominal deck Y=4..6 subject to later bank / water design |
| City-side approach reserve | X=330..390, near Z=100; curved accessible approach to be designed |
| Far-side approach reserve | X=630..710, near Z=100; curved accessible approach to be designed |

The bridge ground-reference Y=0 is not a final deck elevation. Future water should be recessed below the existing Y=0 bank datum; terrain must then be locally cut or rebuilt to avoid grass drawing through water. No raised rectangular platform or replacement ground patch is used now. The 240 m span and 180 m river width are scene-planning dimensions, not asserted real bridge measurements.

## Ground preparation implemented in Part 1

Old ground: X=-1500..300, Z=-1000..950, Y=0.
New ground: X=-1500..800, Z=-1000..950, Y=0.

The existing 100 x 100 base mesh is transformed with center (-350,0,-25), scale (23,1,19.5). Only the eastern edge moves (+500 m); west and Z boundaries stay fixed. The existing world-position-based grass/checker appearance, normals, lighting, shaders, two triangles and one terrain draw are retained. No new terrain overlay, mesh allocation per frame, draw call or shader path is introduced. The far plane remains 1000 m; the proposed bridge center is about 520 m from Baiterek and can be approached using the new flight controls.

## Future bridge geometry plan only

Use a sampled smooth centerline and a shallow continuous vertical curve for the broad pedestrian deck. Sweep a partly enclosed asymmetric canopy section over it, with an open river-facing side and clean abutment transitions. Separate primary longitudinal ribs, secondary diagonals and perimeter members into static material batches. Generate a consistent triangular lattice from shared surface topology, deduplicating shared edges so intersecting triangles do not duplicate beams. Keep longitudinal sections moderately spaced and reuse curved beam helpers; add finer detail only where it materially affects pedestrian views. Avoid per-beam draw calls and transparent panel sorting unless later reference review requires glazing. Supports should connect into abutments/selected piers without blocking the walk or bank promenade.

The user's bridge photos are not present in this workspace (`references/` is absent). Canopy proportions, handedness and final lattice density must be checked against those photographs before bridge modeling; no exact visual match is claimed at this planning stage.


## Part 2 ? built river and natural banks

The approved +X site is retained. `EsilRiver` uses shape-preserving cubic Hermite curves (equivalent cubic segments) for the centerline and width, with both banks derived from that same parameterization. Eight centerline/width controls span Z=-600..700. Water widths vary from 160 to 180 m. The centerline stays exactly X=510 across Z=0..200 for the future bridge.

| Built element | Dimensions / world bounds |
| --- | --- |
| Main river reach | Z=-600..700; approximately 1302.8 m along the sampled centerline |
| Main-reach width | 160..180 m across X; gentle meanders |
| Water including rounded terminal transitions | X=395..607, Z=-626..726 |
| Water elevation | Y=-3.6 m, opaque with shader-only ripple/reflection variation |
| Upper bank / terrain cut footprint | X=371..631, Z=-650..750 |
| Bank cross-section | 24 m horizontal transition from waterline to Y=0, with eased top and toe; 2 m submerged toe reaches Y=-4.04 |
| Typical bank grade | 3.6 m rise over 24 m (average 1:6.67; maximum approximately 1:4.44) |
| At future bridge Z=100 | Water edges X=420/600; upper bank edges X=396/624 |
| Bridge approach land | Proposed bridge ends X=390/630 stay 6 m behind the upper bank edges |
| Promenade reservation update | Use 8 m-wide corridors 8..16 m behind each upper bank; near Z=100 these are X=380..388 and X=632..640 |

The earlier fixed promenade examples X=400..408 and X=612..620 are superseded by these bank-relative reservations, since those earlier lines would lie on the new slopes. No promenade geometry was created in Part 2; Part 3 below implements these reservations. The city-side flat bank reserve is at least 41 m wide within X=330..710, and it widens to 66 m at the bridge; the far side has at least 79 m, widening to 86 m at the crossing.

The finite water reach has smoothly rounded, shallow elliptical ends (26 m beyond each main endpoint). Their offset banks consume the 50 m terminal reserves and return to Y=0 without vertical end walls. Later river extensions can continue from the main endpoint sections instead of these terminal transitions.

The original continuous flat ground draw is replaced by one static indexed mesh outside the actual curved bank footprint. It keeps the original X=-1500..800, Z=-1000..950 bounds and Y=0 surface outside that footprint. Grass shading is unchanged; there is no raised patch, duplicate ground under the water, or shader-based global ground removal. The old shared ground utility remains available to unrelated code.

Rendering: replacement terrain 1,304 triangles / one draw; banks 10,368 triangles / one draw; water 1,168 triangles / one draw. Total 12,840 triangles and three draws, a net addition of 12,838 triangles and two draws compared with the previous two-triangle ground. All three ranges share one initialization-only VAO/VBO/EBO (about 0.43 MiB). Opaque water and bank materials are scoped by a uniform that is reset before other scenery. Existing glass passes, caches, vehicle instancing, camera controls and far plane are unchanged.


## Part 3 ? pedestrian embankments and Key 9

`EsilEmbankment` adds static, material-batched geometry beside the approved river. The river constructor, bank/water geometry, shaders and render methods are unchanged; three read-only layout helpers expose flat offset curves and the existing slope surface height.

- Main promenades: 8 m wide, from 8 to 16 m behind the upper bank edge, at Y=0.04. West centerline length approximately 1305.06 m; east approximately 1301.67 m.
- Rounded terminal links join both banks on existing ground: approximately 290.59 m at the -Z end and 300.01 m at the +Z end. The complete walking circuit is approximately 3197.33 m. The 8 m path width is maintained through the links.
- Bridge approaches: smoothly widen through Z=74..126. The central Z=88..112 area is 20 m wide across X on each bank (upper-bank offsets 4..24). At Z=100 the west paving spans X=372..392, east X=628..648; future bridge endpoints X=390/630 lie inside these pads. No curbs, railings or stairs cross the two bridge mouths. The bridge itself remains unbuilt.
- Edge finish: 0.32 m-wide path kerbs rise to Y=0.10; restrained 0.42 m-wide coping follows the bank top at Y=0.14. Openings are left at bridge and stair connections. There are no full-height concrete walls along the river.
- Three 5 m-wide descents: west Z=35, east Z=165, west Z=250. Each has ten 0.11 m risers and 0.45 m treads. Upper landings meet the promenade; lower viewing platforms are at Y=-1.06, 2.54 m above the water, and stop 12.5 m back from the waterline. Short stone sidewalls support them above the unchanged slopes. Slim 1.05 m guardrails protect the lower platforms and descent edges; shared railing posts are not duplicated.
- Pavement: staggered 1.2 x 0.6 m rectangular light-gray stone slabs, subtle per-slab variation and joints, generated in the shader rather than individual tile objects. A local, restored polygon offset prevents distant ground/paving z-fighting; no bank or water depth state is changed.
- Key 9: eye (384,3,100), target (510,-1,100), yaw 0 degrees / pitch approximately -1.82 degrees. This is on the solid west approach, looking +X across the future bridge alignment. The existing teleport edge handling and camera flight mode are preserved.

Added rendering cost: 14,850 triangles, 29,700 vertices, three opaque material draws (paving 1,492 triangles; stone 12,206; railings 1,152), approximately 1.08 MiB of static geometry. No new texture, alpha-sorting pass, per-frame mesh construction or instance-buffer change. The original river module still costs 12,840 triangles and three draws.

Debug x64 build and mesh/OpenGL checks passed. Inspected elevated, Key 9, stair and end-link views; verified the teleport in walking and fast-flight modes. The approved river construction/render code and bank/water shader were compared unchanged; 10,000 central water pixels match the previous capture. Existing controls, title FPS/movement display, landmarks, glass caches and vehicle instancing remain untouched.


## Part 4 ? Atyrau Bridge structural base

The bridge is generated procedurally in `AtyrauBridge.cpp/.h`. It follows two joined cubic Bezier segments with a continuous tangent: west landing (391.5,0.04,100), an S-shaped excursion toward Z=107.11 and Z=92.89, and east landing (628.5,0.04,100). The ends are 1.5 m inward from the preliminary X=390/630 proposal so the completed curved centerline stays within the requested approximate 210..240 m length. Both end cross-sections fit within the approved 20 m-wide paved bridge approaches (Z=88..112); each overlaps the pad by at least about 0.5 m. No approved river, slope, coping, promenade or approach mesh is modified.

| Item | Built dimensions |
| --- | --- |
| Three-dimensional centerline length | Approximately 239.38 m |
| Plan length / X projection | Approximately 239.24 m / 237 m |
| Overall deck width / clear walking surface | 12 m / 11.2 m between raised edge strips |
| Deck surface | Y=0.04 at both ends; Y=3.64 at center (510,3.64,100) |
| Vertical profile | Smooth sin-squared rise, applied by longitudinal X position; maximum grade approximately 4.77% |
| Slab / structural depth | Main slab 0.38 m; plates taper locally at the landings to avoid intersecting the existing terrain. Paired longitudinal box girders deepen toward midspan; maximum combined depth approximately 1.68 m |
| Temporary railings | Four horizontal silver-gray rails on each side, repeated posts, top approximately 1.23 m above deck; no diagonal lattice |
| Maximum height including railings | Approximately Y=4.87 |
| Minimum superstructure clearance above water | Approximately 3.00 m near the shore; 5.56 m beneath the midspan girders |

Two bank abutment assemblies lie at approximately (410,102.13) and (610,97.87) in X/Z, with foundations embedded into the existing slope. Two in-river supports lie at approximately (470,106.81) and (550,93.19), producing principal spans of about 60 / 80 / 60 m in X. Each in-river assembly has a tapered stem, two restrained spreading support arms, bearing seats and submerged footings at Y=-7.2..-6.0. This is a bridge foundation design datum; the existing river has no modeled or surveyed bed at that depth. Water/bank surfaces are unchanged. Steel bearing pads contact the girder undersides; bank seats support the end spans. Short transverse diaphragms connect the paired longitudinal boxes.

`AtyrauAlignment::sample(t)` exposes the finished deck center, tangent, horizontal right vector and upward normal for later stages. `parameterAtX(x)` locates a station. The future tunnel-like canopy can follow these frames over the central 11.2 m walking zone, with an initial planning allowance up to about 7 m above the deck; no canopy ribs, panels or lattice are built in this stage. Reference photographs remain absent locally, so exact photographic matching is not claimed.

The walking camera now reads a small cached table built from the bridge's actual deck triangles. It remains 3 m above the deck while walking across it, and smooth flight-to-walk descent targets the deck when overhead. Outside the bridge footprint the original flat-world walking datum and all key bindings, teleports, mouse behavior and flight speeds remain unchanged. The CPU surface table and its bounding boxes are prepared once during scene construction; there is no per-frame mesh generation, allocation, GPU upload or trigonometric rebuilding for this query.

Added rendering: 20,440 triangles / 39,768 vertices / four opaque material draws (walking 384, silver edges/rails 16,676, structural underside/girders 2,852, concrete supports 528). Geometry uses approximately 1.45 MiB of static GPU buffers; the small CPU walking table is additional. The deck reuses the existing stone paving shader, while steel and concrete use existing opaque daylight shading. No shader edits, transparency passes, glass cache changes or vehicle updates are introduced. The landing paving bias is local and restored.

Verification: Debug x64 build passed without new C++ warnings. Constructor checks positions, unit normals, triangle winding, deck/terrain clearance and grade. A separate static audit sampled the centerline and understructure, and measured the dimensions above. Full-scene elevated, Key 9, underside and east-landing renders passed shader and OpenGL-state/error checks. A focused camera test crossed the complete bridge on foot and verified smooth midspan landing from flight, existing Key 9 behavior and all 5/15/25/50 m/s navigation modes. No benchmark suite was run. No commit or push.


## Part 5 ? primary sculptural metal canopy

`AtyrauCanopy.cpp/.h` adds a separate static steel mesh following the approved `AtyrauAlignment` frames. The existing bridge base, piers, deck, rails, walking support, river, banks, promenades, shaders and camera/input modules are unchanged. Local bridge reference photos are still unavailable; this is an interpretation of the supplied architectural description, not a claim of exact photographic matching.

The envelope is a smoothly varying asymmetric oval arch with a broad, relatively flat crest. Its feet stay at transverse offsets +/-5.94 m on the raised deck edges. Above those feet the section flares outward, with a varying crown shift and mild cross-sectional lean. Height and width vary smoothly with longitudinal X position instead of forming a constant circular tunnel.

| Element | Built specification |
| --- | --- |
| Length along approved deck alignment | Approximately 239.38 m |
| Envelope width | Approximately 15.80..17.48 m; feet remain within the 12 m deck width |
| Maximum world height including steel | Y=11.20 m |
| Clear height beneath crown framing | Approximately 6.34..7.38 m above deck |
| Conservative minimum clearance over the full 11.2 m walking zone | Approximately 5.07 m |
| Primary transverse ribs | 31, at approximately 7.9 m X intervals / 8 m along the curve |
| Continuous longitudinal members | Five, following q=0.12,0.31,0.50,0.69,0.88 of the same envelope |
| Steel sizes | Main ribs diameter170 mm; longitudinal members130 mm, crown member150 mm |
| Intersection finishing | 155 integrated collars (up to220 mm diameter), formed by varying the rib sweep itself |
| Deck attachment | 62 short steel curb seats; rail/foot spacing at mounting region approximately45 mm |

The primary members are hollow-section-like opaque steel sweeps with parallel-transported frames, correct curved surface normals and only complete-member end caps. Longitudinal samples include the exact rib stations, so their intersections coincide. Collar geometry is part of each rib rather than an additional pile of spheres or overlapping tubes. The existing silver-metal material is reused without a new shader or light. No opaque walls, panels, roof skin, diagonal lattice or transparent glazing is added. Large open bays remain for the next stage.

`AtyrauCanopyShape::surface(t,q)` exposes the unmeshed parametric envelope for later infill. q=0 and1 are the curb attachment curves; q=0.5 is the crown. All steel and anchor geometry is generated during construction and uploaded once. No per-frame geometry, buffer creation/upload, sorting or allocations are introduced by the canopy renderer.

Added rendering cost: 51,432 triangles, 27,480 vertices, one opaque draw, approximately1.43 MiB of static GPU geometry. The approved bridge base remains20,440 triangles/four draws. No new textures or transparency passes are needed, and all MEGA/EXPO glass caches and vehicle instancing remain unchanged.

Verification: Debug x64 build passed without new C++ warnings. Constructor checks finite vertices, normalized normals, winding, seated feet, pedestrian clearance and actual low-level rib/rail separation. Full-scene elevated, side, Key9 and interior pedestrian captures passed shader/link/OpenGL-error and depth/blend/material-state checks. Exact generated steel and anchor checks found no railing intersection; the mounting-region gap is approximately0.045 m. All existing source outside Scene/project integration is preserved by a start-of-task hash guard. No performance benchmarks were run. No commit/push.


## Part 6A ? open triangular steel lattice

The approved `AtyrauCanopyShape::surface(t,q)`, primary ribs and spines are preserved exactly. The infill is generated on the same envelope and appended to the existing canopy buffers after the original 51,432-triangle primary range. A frozen pre-task generator was compiled in a diagnostic executable; hashes of its generated primary vertices and indices match the current primary prefix. No bridge alignment, envelope dimension, deck, pier, railing or waterfront geometry was changed.

- Two crossed diagonal families: 600 members each, 1,200 total, in 60 longitudinal rows and 10 cross-sectional bands. Longitudinal rows are approximately 4 m apart; every even row coincides exactly with an existing primary rib. Cross-sectional nodes include all five approved longitudinal member positions.
- Thirty lighter transverse ties fill the half-bay rows without duplicating the 31 original primary ribs. Sixty new curb seats support their feet, using the same mounting geometry and rail clearance as the primary structure.
- Both diagonal families share exact (t,q) crossing samples, giving 600 coherent welded crossing locations. Fine members are 70 mm in diameter, with restrained 84 mm integrated weld swells at crossings; secondary ties are 90 mm in diameter. The existing primary sections and collar sizes remain unchanged.
- Six-sided curved steel sections keep the infill economical. Only six segments are needed per short diagonal, sampled on the approved curved surface. Diagonal end rings are embedded in ribs/ties, so hidden end caps are omitted (14,400 triangles saved). Exposed secondary tie feet remain capped and mounted.
- Triangle connections are checked against the actual sampled primary rib and spine polylines, not only the analytical envelope. Each endpoint lies inside the supporting member's thickness. Crossings, railing separation, finite coordinates, unit normals, winding and degenerate triangles are validated during initialization.
- The secondary rods reuse the existing opaque metal shading style with a slightly lighter silver-white tone (local `esilSurface=6`). The old primary metal, river, bank and other city material bodies remain unchanged. Both ranges reset the material flag after rendering. There are no panels, glass, alpha sorting or opaque roof skins.

Added infill geometry: **114,840 triangles, 66,120 vertices, one opaque draw**, approximately 3.33 MiB of static GPU data. This includes 1,200 diagonals plus 30 continuous secondary ties and 60 small seats (1,230 added linear lattice members). Total canopy cost is now 166,272 triangles, 93,600 vertices and two opaque material draws. All geometry is constructed/uploaded once; no buffers are recreated or updated per frame.

An audit of generated infill vertices over the rendered walking surface found a minimum clearance of approximately 5.22 m. The original primary framework remains the lower limiting structure (approximately 5.07 m overall). The existing bridge-aware walking behavior, Key9 and all navigation modes remain unchanged.

Debug x64 build passed without new C++ warnings. Full-scene elevated, Key9, side and pedestrian captures passed shader compilation/link, OpenGL-error and depth/blend/material-state checks. Visual inspection confirmed an open woven/triangular pattern across the roof and both sidewalls, with the river and sky visible through it. Reference photos are still absent locally, so exact photographic resemblance is unverified. No benchmark suite was run; no commit or push.

## Part 6B - clearer architectural lattice

The fine appearance came from 1,200 thin diagonals arranged as crossed X cells, with 600 additional crossing nodes between the primary framework. The replacement uses staggered triangular fans: even stations connect the six bands' boundaries to shared half-arc nodes on each intervening tie. Half-arc nodes are measured along the asymmetric section instead of using parameter midpoints. This produces a consistent repeating pattern without tiny crossed cells; the five existing longitudinal spines define the band boundaries.

There are now 720 diagonals (360 in each family), 30 secondary transverse ties and 180 shared nodes. The 60 longitudinal half-bay intervals remain approximately 4 m along the alignment. Diagonal diameter increases from 70 to 90 mm; secondary ties are 95 mm with restrained integrated 110 mm connection collars. Six-sided sections and six curved segments per diagonal keep the geometry economical. No separate high-poly joint objects are added. The existing 60 secondary mounting seats remain.

The approved analytical envelope, primary ribs, spines, collars, deck, supports and alignment are unchanged. Secondary steel is seated up to 25 mm outward from the analytical surface, smoothly fading to zero at the feet, to retain headroom with the thicker sections. Endpoints are checked against the actual primary polylines and remain embedded in their supporting steel. Generated infill vertices over the walking surface have a measured minimum clearance of 5.241 m (previously approximately 5.22 m). The unchanged primary framework remains the overall limiting structure at approximately 5.07 m.

Infill cost decreases from 114,840 to **82,440 triangles** (28.2% fewer), with **47,040 vertices and one opaque draw**. Combined canopy cost is 133,872 triangles, 74,520 vertices and two opaque draws; the original primary range remains 51,432 triangles. Static infill GPU data decreases from approximately 3.33 to 2.38 MiB. Rendering, material flags, glass caches and vehicle instancing are unchanged; all lattice geometry is generated and uploaded once.

Debug x64 build passed. A frozen primary generator's vertex/index hashes still match the current primary mesh. Initialization and diagnostic checks passed for finite geometry, unit normals, winding, degenerate triangles, attachments, clearance and railing separation. Full-scene elevated, side, Key9 and interior pedestrian renders passed shader/link, OpenGL-error and depth/blend/material-state checks on Intel UHD Graphics. Visual inspection found fewer small crossings and a more open, readable triangular tunnel. Local reference photographs remain unavailable, so exact photographic matching is not claimed. No benchmark suite, commit or push was performed. Awaiting user F5 review.

## Part 6C - central woven scale enclosure

The primary canopy and all 720 approved diagonals remain intact. A frozen Part 6B generator's complete vertex/index hashes match the new generator's original steel prefix exactly. The existing bridge deck, supports, safety rails, alignment, landings, envelope, river, promenades, controls and other districts are unchanged.

The dense layer is concentrated around the center instead of enclosing the full 239.38 m bridge. Its support region runs between primary ribs 6 and 24, at X=438.9..581.1 (142.2 m in X). Visible sheet bounds are approximately X=448.38..571.58, or about 124 m along the curved alignment. Roughly 57 m of each approach remains panel-free. The full-size core is approximately X=470.5..549.5; sheet dimensions decrease smoothly toward both enclosure ends, retaining daylight and river views through the smaller transition scales.

620 shallow folded diamond sheets form 20 staggered longitudinal lanes. Lane spacing follows equal physical arc intervals around the asymmetric midspan section, with alternate lanes shifted half a longitudinal row. Centers are approximately 3.95 m apart along each lane; full-size sheets are 3.6 m long, with transverse dimensions following the local section. Small diagonal slots separate the panels. Each sheet has four folded facets on both faces, closed 30 mm edges and up to 160 mm of fold depth, costing 16 triangles. They are opaque ivory metal, not rectangles, glass or thin rods. The crown and both sides use the same continuous pattern.

The sheet layer is seated inside the approved rods so the rods cannot cut dark fragments through the interior facets. Twenty narrow 50 x 32 mm carriers follow the same envelope; 380 short rectangular stand-offs connect them to the 19 central primary ribs. Sheet tips meet the carrier sweep at sampled stations. Near-duplicate stations within 2 mm are merged to prevent zero-area chords after float-precision alignment inversion. Carrier endpoints are seated on existing ribs. The layer remains clear of the walking corridor and original railings; actual generated-vertex checks measured at least 5.422 m above the rendered walking surface. The old lattice remains at 5.241 m and the unchanged primary structure remains the overall limit at approximately 5.07 m.

Rendering adds 9,920 sheet triangles and 22,240 carrier/stand-off triangles, or **32,160 triangles / 74,240 vertices / one additional opaque draw** (approximately 2.63 MiB of static GPU geometry). Total canopy cost is now **166,032 triangles / 148,760 vertices / three draws**: primary steel 51,432; lattice plus carriers 104,680; ivory sheets 9,920. Carriers share the original infill material range; sheets use one new range in the same VAO/VBO/EBO. All geometry is initialized and uploaded once. No per-frame rebuilding, transparency, sorting, buffer updates or independent per-panel draws are introduced.

The only shader addition is local `esilSurface=7`, an opaque ivory material with restrained two-sided daylight fill so downward-facing folds remain bright. Existing material branches, MEGA/EXPO glass caches and vehicle instancing are unchanged. Shader flags and object-color state are restored after rendering.

Debug x64 build and initialization checks passed. Full-scene aerial, side, Key9 and interior renders passed shader/link, OpenGL-error and depth/blend/material-state checks on Intel UHD Graphics. The interior view was taken 1.7 m above the deck and shows a bright scale-lined passage without obstructions; aerial/side views show the central concentration and open approaches. Local user reference photos remain unavailable, so this is an interpretation of the supplied description and exact photographic matching is unverified. No performance benchmark suite was run. No commit or push; awaiting F5 visual review.

## Part 6D - graduated open entrances and portal finishing

The approved 620 central sheets, their carriers, all 720 diagonals, primary ribs/spines and the analytical envelope are preserved exactly. A frozen Part 6C generator's complete vertex/index hashes match the current generator before the new details are appended. A second hash check reconstructs the original index order after batching and verifies that none of the original vertices or triangles changed. No bridge base, support, river, embankment, approach, shader, camera or unrelated scene code changed in this task.

198 additional folded sheets (99 at each end) continue the original 20-lane staggered phase into previously empty cells. Both ends use the same progression by distance from the entrance. New scales first appear around the high crown, then spread toward the shoulders closer to the center; shoulder lanes emerge later to retain clearance under the lower approach envelope. All eligible adjoining lanes are filled, without random omissions. Their dimensions and fold depth increase smoothly toward the center, using the approved diamond geometry, 30 mm thickness and unchanged ivory material. Cells already occupied by the original 620 sheets are explicitly excluded.

The new transition areas extend approximately 40 m into each formerly bare approach. Overall sheet bounds now reach approximately X=409.28..610.66, leaving about 18 m at each entrance free of panels. The original approximately 124 m enclosure remains untouched. Narrow carrier extensions terminate on original primary ribs and join the central carriers without duplicate end caps. Their outermost caps reuse the exact swept hexagonal rings. Lanes with existing carrier support need no extension. Short stand-offs connect the added carrier sections to the primary ribs.

Both terminal ribs receive a continuous bevelled metallic portal sleeve, tapering from about 260 mm section depth at the crown to 110 mm at the feet. The longitudinal profile is 280 mm deep, with restrained outward seating to preserve pedestrian and railing clearance. The sleeves finish the open entries while keeping the original oval sections, feet, deck and railings. The existing 720-diagonal pattern is already complete, so no extra diagonal families are added.

New geometry measured at least 5.234 m above the rendered walking surface; rail separation, finite coordinates, unit normals, winding and nondegenerate triangle checks passed. The unchanged original primary structure remains the overall limiting clearance at approximately 5.07 m.

Added rendering cost is **12,920 triangles / 19,036 vertices / zero additional draws**, approximately 0.73 MiB of static GPU data: 3,168 sheet triangles and 9,752 carrier/stand-off/portal triangles. The canopy now contains **818 sheets, 178,952 triangles, 167,796 vertices and three opaque draws**. The original ivory and secondary steel ranges absorb all additions in the existing VAO/VBO/EBO. Geometry and index batching run only at initialization; no new transparency, sorting, per-frame allocations or uploads are introduced.

Debug x64 build passed without new C++ compiler warnings. Aerial and both entrance renders (including views from outside the portal frames at 1.7 m above the approach paving) passed shader/link, OpenGL-error and depth/blend/material-state checks on Intel UHD Graphics. Visual inspection confirmed a gradual crown-to-shoulder progression, open river sightlines and clean continuous portal edges. MEGA/EXPO glass caches, vehicle instancing and all controls are untouched. No performance benchmarks, commit or push. Awaiting user F5 review.

## Part 7A - stone deck, silver railings and portal base finishing

The approved canopy module is unchanged, preserving all 818 sheets, lattice, ribs, transitions and bevelled portal outlines. The bridge's original walking positions, normals and triangle topology are also unchanged. Only walking UVs, railing geometry and small portal base fittings change in the bridge mesh. The analytical alignment and cached walking-height implementation remain exact.

The former world-axis paving grid is replaced with bridge-local metre coordinates: transverse position and accumulated three-dimensional centerline distance (239.378 m over the original 192 segments). A local opaque stone material (`esilSurface=8`) produces staggered 0.60 x 1.20 m slabs, nominal 5 mm joints, restrained warm-gray variation and very subtle grain. Both sides have 400 mm stone border courses at offsets 5.02..5.42 m, followed by 180 mm darker trim up to the unchanged 5.60 m walking edge. All pattern details live on the original surface; no overlay, paving mesh, extra draw or raised band is added. Other promenade and city materials retain their previous shading.

The two top handrails are now continuous 64 mm diameter, eight-sided sweeps with smoothly shaded normals and exact sealed end rings. Their maximum height remains approximately 1.228 m above the deck. The three lower rails on each side use slimmer 36 x 40 mm sections at their original heights. All 162 posts retain their approved stations and offsets, with 55 mm square profiles, embedded 120 x 110 mm baseplates and shallow 80 x 70 mm saddle fittings. The metal range now uses the existing silver-white secondary-steel material, making rails and trim consistent with the canopy without a new reflection pass.

Four low metal shoes finish the existing portal feet on the raised edge strips. The already approved bevelled entrance sleeves remain untouched, with no new rings, walls or doors. Both landings retain Y=0.04 and their original connection to the embankment paving; local 25 mm flush gasket strips in the deck material mark the movement joints. The existing depth-bias restoration is preserved. There are no new slabs, terrain edits, steps, water changes or promenade-width changes.

The revised railing/foot fittings remain outside the 11.2 m walking corridor. A generated-vertex audit measured a minimum approximate transverse distance of 5.738 m from the centerline. The approved overhead clearances remain unchanged because the canopy and walking heights are unchanged.

Bridge cost changes from **20,440 to 27,472 triangles** (+7,032) and from 39,768 to 44,604 vertices (+4,836), with **four opaque draws unchanged**. The additions require approximately 0.23 MiB of static GPU buffers. Including the unchanged canopy gives 206,424 triangles and seven draws. Geometry, accumulated UVs and all fittings are prepared only at initialization; no glass sorting, vehicle instancing, per-frame allocations, uploads or camera code changes are introduced.

Debug x64 build passed without new C++ compiler warnings. A frozen pre-task bridge generator verified identical walking positions/normals/topology, underside/girder and pier/abutment batches, side fascias and bearing geometry. UV monotonicity, landing elevations, railing clearance, finite coordinates, unit normals, winding and nondegenerate triangle checks passed. Full-scene interior and exterior entrance renders at pedestrian height passed shader compilation/link, OpenGL-error and depth/blend/material-state restoration checks on Intel UHD Graphics. Visual inspection found continuous curved paving, refined railings and clean landings; no new rendering issue was observed in those views. No performance benchmark suite, commit or push. Awaiting F5 review.

## Part 8A - riverside trees and low planting

`EsilEmbankment` now owns five additional static landscape material ranges in its existing buffers. The three original paving, stone and metal batches remain byte-for-byte identical to a frozen pre-task generator, including the promenades, coping, stairs, viewing platforms and rails. River, terrain, bridge, all 818 canopy panels, shaders, Scene integration, camera controls, glass caches and parked vehicles are unchanged.

The main planting runs along both existing curved bank edges from approximately Z=-574 to Z=681. Most trees stand on flat land behind the promenades, typically 22..25 m outward from the upper bank edge; four grove areas per bank extend farther into the landward lawns. Slender ornamental trees also occupy selected pockets in the level strip at offset 4.2 m, never the recessed slopes. Tree spacing varies intentionally, and collision checks keep their crown bounds separated. Open view/future-furniture windows around Z=-300, -20, 340 and 600 interrupt the rows.

The bridge mouth stays open through Z=72..128. Matched tree groups around Z=56/64 and 136/144 and four low approach beds at Z=64/136 frame the banks without occupying the paved landings. All three stair/viewing openings retain clear buffers. The planted footprint and crown bounds are checked against the actual curved promenade edges, including their local bridge widening and curbs. A generated-vertex audit found approximately 2.161 m minimum separation from pavement edges, with no planting in water, on slopes, across bridge sightlines or inside stair openings.

Built quantities:

| Element | Count / form |
| --- | --- |
| Trees | 157: 77 rounded deciduous and 80 slender ornamental; 79 west / 78 east |
| Approximate tree heights | 6.3..8.5 m; restrained crowns using shared `Tree::foliageOffsets` and `foliageRadii` |
| Low shrubs | 238, in organized groups of seven per bed; approximately 0.60..0.67 m tall |
| Ornamental grasses | 170 tufts, five folded opaque leaves each; approximately 0.47..0.57 m tall |
| Organic planting beds | 34, 17 per bank, including four near the bridge; rounded irregular soil outlines with small closed earth edges |

Trees reuse the existing four-cluster crown description, baked into economical indexed ellipsoids with correct ellipsoid normals and unique pole vertices. Tapered trunks and short branches connect the crowns. Low foliage uses smaller indexed forms; grass leaves are shallow solid ribbons without alpha cards or coplanar back faces. Soil and bark share one earth-toned batch, with two crown greens, one shrub green and one muted grass material. Geometry is constructed and uploaded once. There are no per-tree draws, per-frame placement/LOD loops, new transparency passes, shader edits, furniture, lights or paths.

Added cost: **109,152 triangles / 95,522 vertices / five opaque draws**, approximately 4.16 MiB of static GPU buffers. The embankment module totals **124,002 triangles and eight draws**, including the unchanged original 14,850 triangles / three draws. Vegetation color/material flags and polygon-offset state are restored after rendering. The limited static mesh cost did not require a new distance-based LOD system; visible F5 FPS still needs user review and is not claimed from the diagnostic captures.

Debug x64 build passed without new C++ compiler warnings. Native checks passed for old-mesh preservation, placement, finite positions, unit normals, winding and nondegenerate triangles. Aerial and pedestrian waterfront views near the bridge passed GLSL compile/link, OpenGL-error and depth/blend/material-state checks on Intel UHD Graphics. Visual inspection confirmed open lawns, unobstructed paths and mixed tree forms along both banks. The initial aerial inspection camera exceeded the existing 1,000 m view range; it was repositioned for inspection without changing application projection or fog. No performance benchmark suite, commit or push. Awaiting F5 review.

## Part 8B - seating, pedestrian lighting and resting bays

Both banks now have modern furniture in the existing reserved viewpoint areas. The approved 157 trees, 238 shrubs, 170 grass tufts and 34 beds retain their exact generated geometry. The river, slopes, Atyrau Bridge and its 818 panels, existing promenade top meshes, stairs/platforms, rails, shaders, controls and other city modules are unchanged. A temporary planting-bound registry is collected during initialization without changing plant generation; it is used to validate furniture, then discarded.

| Addition | Count / placement |
| --- | --- |
| Benches | 36: 18 per bank; four per rest bay plus two on each existing widened bridge forecourt |
| Pedestrian lights | 52: 26 per bank, approximately 50 m spacing; a station near the western stair is adjusted locally |
| Waste bins | 20: two per bay plus two per bridge forecourt |
| Resting bays | Eight: one at Z=-300, -20, 340 and 600 on each bank |

The approximately 12 x 5.7 m chamfered bays sit on flat land outside the promenade, with matching stone paving at the existing Y=0.04 datum. Each bay has a pair of river-facing benches and two inward-facing side benches, leaving its center open. Low edging follows the chamfered outline. Ten-metre local openings in the outer curb connect each bay through a flush 320 mm-wide stone threshold. Original paving is not overlaid or rebuilt; bay/threshold vertices share its existing five-metre boundary stations to prevent cracks. Original curb alignment elsewhere is preserved, and all stair/viewing-platform coping remains unchanged. The flush openings are the only necessary edits to pre-existing edge geometry.

The four approach benches stand at Z=84/116 in the existing wider forecourts; bins are at Z=82/118. All stay outside the bridge's Z=88..112 entrance axis and outside the normal eight-metre walking corridor. Furniture placement checks use full plan bounds, existing plant/crown/bed bounds, other furniture, stair openings, terrain limits and paved footings. The 108 sites passed these checks without relocating any vegetation. Pole stations sit outward from the curb; those adjoining a rest bay move to its outer edge. Luminaires point toward the walk, not into tree crowns.

Benches reuse `Bench::frameTransforms()` and `slatTransforms()` baked into dark-metal and wood batches. The approximately 5.2 m lights reuse `StreetLight` component transforms and opaque, non-emissive diffuser geometry; there are no dynamic lights or shadows. Restrained twelve-sided bins have dark tapered bodies, finished metal lid rims and shallow recessed openings without transparency. All furniture is grounded on its existing paving or lawn datum.

Added cost is **12,408 triangles / 25,368 vertices / three opaque draws**, approximately 0.92 MiB of static GPU geometry. The embankment module totals **136,410 triangles / 150,590 vertices / eleven draws**. Bay paving, edging, poles and bin rims join existing batches; only dark furniture metal, wood and daylight diffuser materials add ranges. No per-object draw calls, new shaders, per-frame geometry/allocations/uploads, transparency or changes to glass caches or vehicle instancing are introduced.

Debug x64 build passed without new C++ compiler warnings. Native frozen-generator checks verified identical approved vegetation, original paving and railing geometry; coping outside the eight local rest-site regions also matches exactly. Placement, finite vertices, unit normals, winding and nondegenerate triangle checks passed. Full-scene aerial and pedestrian rest-bay renders passed GLSL compile/link, OpenGL-error and depth/blend/material-state checks on Intel UHD Graphics. Visual inspection confirmed clear paths, clean bay connections and coordinated seating/lights. No FPS benchmark suite, commit or push. Awaiting F5 review.

## Part 8C - viewing terraces, curved garden routes and flower gardens

Four shallow oval terraces occupy flat land between the existing promenades and the upper bank edges. Their nominal footprint is 15 m along the river and 5.8 m across it, staying at least 0.75 m landward of the upper slope. All tops remain at Y=0.04 on solid ground. Light stone paving, low trim and restrained 1.05 m-high waterside railings reuse existing materials and mesh helpers. The terrace near Z=-20 on the east bank gives an unobstructed oblique view of the white Atyrau canopy; no river, slope or bridge geometry is modified.

| Terrace | Center X / Y / Z |
| --- | --- |
| West, southern viewpoint | (375.645, 0.04, -300) |
| West, northern viewpoint | (409.756, 0.04, 340) |
| East, bridge viewpoint | (623.810, 0.04, -20) |
| East, northern viewpoint | (630.762, 0.04, 600) |

Eight secondary connections are generated from sampled cubic Bezier curves. Four 3 m-wide landward garden routes connect the existing resting bays: approximately 329.5 / 293.7 m on the west bank and 330.7 / 295.4 m on the east bank. They bow beyond the existing tree groups, linking Z=-300 to the promenade at Z=-10 and the resting bays at Z=340 and 600. Four deliberately short, direct 3.4 m-wide terrace spurs (approximately 1.53 m each) connect the terraces across the narrow level green strip. Total new path length is approximately 1,255 m. Existing normal eight-metre walks remain unobstructed.

Small flush openings are cut only where these connections join existing curb edges: four waterside mouths, two promenade mouths and six resting-bay rear exits. Old paving geometry is neither regenerated nor overlaid. Shared five-metre edge chords and exact resting-bay boundary chords prevent cracks or z-fighting. New paths have light stone surfaces, narrow matching borders and grounded edge faces; all connections use the original paving elevation.

Twelve additional ornamental gardens (six per bank) stand beside the garden routes and around the bridge approaches. Four of them form matched approach compositions at Z=76 and 124. Other beds are at Z=-240, -85, 420 and 550. Each bed uses a small organic soil outline, four coordinated elliptical flower bands and one modest riverward hedge group. The new beds contain 1,056 flowers: 264 each red, white, yellow and soft purple. Low, closed petal heads, tapered stems, folded leaves and integrated pollen centers are opaque, economical geometry. Flowers are approximately 0.31..0.39 m tall. Twelve hedge groups comprise 36 rounded segments, approximately 0.5 m high. Existing lawns remain visible between these features.

All approved 157 trees, 238 shrubs, 170 grass tufts, 34 earlier planting beds, 36 benches, 52 lights, 20 bins and eight rest bays retain their positions and geometry. Initialization checks use the original 191 plant bounds and 108 furniture bounds for area and swept-path collision tests. Gardens are also checked against new paths/platforms, and new routes remain within the prepared district. The original vegetation, furniture and paving mesh hashes match a frozen Part 8B generator before the additions; their prefixes match again afterward. Coping away from the small connection regions is unchanged. Existing stairs, low viewing platforms, Key9, other controls, shaders, glass caches, vehicle instancing and all other landmarks are untouched.

Added cost is **66,540 triangles / 92,024 vertices / four opaque draws**, approximately 3.57 MiB of static GPU buffers. The embankment now totals **202,950 triangles / 242,614 vertices / fifteen draws**. Paving, trim, rails, soil, greenery and stems join existing material ranges; only four flower colors add ranges. Generation, collision checks and uploads occur once during initialization. No transparency, dynamic lighting, per-frame rebuilding, sorting or buffer updates are introduced.

Debug x64 build passed without new C++ compiler warnings. Mesh checks passed for finite vertices, unit normals, winding and nondegenerate triangles. Full-scene aerial, pedestrian bridge-view terrace and approach-garden views passed GLSL compile/link, OpenGL-error and depth/blend/material-state checks on Intel UHD Graphics. Visual inspection confirmed coherent connections, clear river/bridge views and restrained small-scale gardens. No issue was observed in these checked views; user F5 review is pending. No FPS benchmark suite, commit or push.
