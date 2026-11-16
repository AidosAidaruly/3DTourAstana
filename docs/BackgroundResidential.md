# City Development Part 4 - Optimized residential infill

Forty new apartments form ten four-building groups outside the six completed
residential parcels. Placements use actual procedural streets and terrain;
no real-world geographic coordinates, terrain extension or new highway is
used. All existing buildings, plazas, parking and landscaping are retained.

## Placement plan

Coordinates below are world X/Z metres. Bounds describe the building plots;
short pedestrian connections extend to the stated existing paved edge.
Exact forty centers, rotations, archetypes and height scales are structured
data in `BackgroundResidentialPlan.h`, rather than individual mesh code.

| Group | Plot X bounds | Plot Z bounds | Court center X/Z | Access endpoint X/Z |
| --- | --- | --- | --- | --- |
| EXPO west south | -1220..-1083 | -325..-202 | -1148 / -261 | -1071.4 / -261 |
| EXPO west middle | -1220..-1083 | -170..-30 | -1148 / -102 | -1071.4 / -102 |
| EXPO west north | -1220..-1083 | 1..135 | -1148 / 64 | -1071.4 / 64 |
| EXPO north west | -1045..-870 | 185..335 | -960 / 256 | -960 / 161.4 |
| EXPO north east | -850..-690 | 185..335 | -767 / 256 | -767 / 161.4 |
| Khan southwest north | -260..-95 | -712..-615 | -185 / -666 | -279.8 / -666 |
| Khan southwest middle | -260..-95 | -825..-728 | -185 / -779 | -279.8 / -779 |
| Khan southwest south | -260..-95 | -944..-840 | -185 / -893 | -279.8 / -850 |
| Southern loop west | -656..-470 | -978..-900 | -563 / -915 | -563 / -885.2 |
| Southern loop east | -468..-282 | -978..-900 | -375 / -915 | -375 / -885.2 |

Western groups meet EXPO's existing west-boulevard sidewalk. Northern groups
meet its north sidewalk. Khan's outer groups use the east side of secondary
street S3; the southernmost uses a clear dogleg via (-267,-893) and
(-267,-850), outside the perimeter bend. The two southern groups meet S2's
south kerb. No new paving enters road asphalt. These are walking links;
new private parking and vehicle courts are outside this stage.

Minimum body-to-body separation is **16 m**, including different groups.
Building aprons and canopies fit the selected free envelopes. Conservative
EXPO and Khan exclusion radii are 230 and 180 m. MEGA's entire reserved site,
the stadium grounds, Botanical Garden, central landmarks, all six existing
residential plots and the river district are excluded. G2's open view axis
and the colonnade sightline are unchanged.

## Architecture and site finishing

| Shared archetype | Count | Floors | Local X/Z footprint | Character |
| --- | ---: | ---: | --- | --- |
| A10 | 6 | 10 | 26 x 22 m | Ivory/gray stone, recessed windows |
| A12 | 10 | 12 | 32 x 20 m | Broad limestone apartment wing |
| C14 | 5 | 14 | 24 x 26 m | Alternating pale facade sections |
| B22 | 8 | 22 | 24 x 24 m | Warm brick, stone piers, stronger crown |
| D8 | 4 | 8 | 30 x 18 m | Compact warm-brick housing |
| C16 | 7 | 16 | 24 x 30 m | Glass-accented residential slab |

The distribution is **28 medium-rise / 8 tall / 4 compact**, or 70/20/10%.
Small 0.96..1.04 vertical scales create heights of **27.57..79.01 m**, while
residential storeys remain approximately 3.22..3.48 m. Rotations turn the
entrances into the shared courts, with staggered centers and varied lengths.

Near geometry reuses the proven medium-detail generator: real apertures,
window returns, metal frames, recessed loggias, opaque reflective guards,
floor bands, canopies and doors. Two setbacks, parapets and small technical
roof housings finish each silhouette. There are no interiors.

Each group has unioned 4 m pedestrian branches and entrance aprons, a small
shared lawn court, three trees and six shrubs: **30 trees / 60 shrubs**.
Approximately 20,440 square metres of paving includes all forty aprons and
access links, not a single raised district slab. Ground paving is Y=.06,
rising gently at entries and retained kerb connections. Tile joints use
the local procedural material; individual stones are not objects.

## Shared geometry and rendering

`MediumResidenceGeometry.h` extracts the existing Part 3B facade routines.
The existing Part 3B uploaded vertex AND index buffers are byte-identical
after this refactor. A3 and EXPO generators remain untouched. The additional
compact material choice is used only by the new background archetype.

Six archetypes have three immutable indexed LOD meshes each. Forty static
instances reference these same buffers; repeated compatible buildings share
instanced draws. Each group/type/LOD VAO references a fixed subrange of one
1,280-byte instance buffer. Group visibility and LOD choose existing VAOs;
there is **no instance regrouping, buffer upload or geometry allocation per
frame**, even while flying. Stationary cameras reuse the culling/LOD result.

A small local OpenGL 3.3 shader carries material colors/IDs per vertex,
combining stone, brick, frames, glazing and roofs into one draw per
archetype/group. It reuses the existing daylight and opaque-glass reflection
techniques without changing shared shaders. Medium/far panes interpolate
daylight reflections from vertices; nearby panes retain fragment highlights.

- LOD0: full medium-detail apertures, frames, loggia depth and doors.
- LOD1: flat framed window surrounds/mullions and selected loggia guards.
- LOD2: lightweight windows/floor divisions and unchanged massing/crowns.
- Near leaves at 130 m, returns at 90 m; far enters at 300 m, returns to
  medium at 245 m. Initial selection uses 110/300 m. Distances are measured
  to the nearest building bounds, with hysteresis.
- Ten conservative frustum AABBs include roofs, trees and access connections.
- Existing depth, blend, program, VAO, polygon offset and generic attribute
  state remain intact. There are no new transparency or lighting passes.

| All 40 visible | Building triangles | Court/site triangles | Total | Draws |
| --- | ---: | ---: | ---: | ---: |
| Near LOD0 | 478,856 | 8,970 | 487,826 | 43 |
| Medium LOD1 | 131,016 | 8,970 | 139,986 | 43 |
| Far LOD2 | 68,688 | 8,970 | 77,658 | 43 |

Unique template triangles across all three LODs: 97,724. Static GPU storage
including all templates, instance data and sites: **10,144,968 bytes**.
The aerial view rendered all forty at far LOD. The representative performance
view rendered 24 new buildings (12 medium / 12 far), 63,826 triangles and
27 extra draws; other groups were frustum-rejected.

## Validation and measured performance

Debug x64 builds successfully with no new C++ warnings. Essential validation
checked all LOD vertices, unit normals, indices and nondegenerate outward
winding; actual placements, protected reservations, terrain limits and
building separation; unioned paving without duplicate surfaces; and zero
new paving/asphalt or paving/building overlap. All unrelated source hashes
retain their starting values. The approved Part 3B geometry is byte-identical.
Glass caches, parked-car instancing/LOD, stadium seats and controls are intact.

Full-scene OpenGL/shader/state checks passed. Inspected: an aerial overview
containing all groups, a courtyard-level residential facade, a view toward
EXPO, a view toward Arena and the matched comparison view. The overview
alone uses a wider validation-camera FOV/far plane; game projection and
camera controls are unchanged. All three LODs and full group rejection were
observed, including changes after camera teleports.

Matched stationary sample, Debug x64 / hidden 1280 x 720 / Intel UHD Graphics,
eye (-1360,180,-90), target (-700,25,-260), ordinary game projection,
vsync off and GPU-synchronized 120-frame timing:

| Sample | FPS | Frame time |
| --- | ---: | ---: |
| Before | 90.64 | 11.03 ms |
| Final implementation | 98.28 | 10.18 ms |

The first medium LOD was too costly in this view and was reduced before
finishing. The final short sample shows no regression. GPU clocks/scheduling
can affect a single comparison; these figures do not establish a guaranteed
speedup or replace the user's visible F5 performance review.

## Remaining open land and files

MEGA's reserved land and the narrow occupied Garden/Northern Lights gaps
remain clear. Land farther west/north has no nearby completed road frontage;
the east/river approach area needs its own access plan. Existing landmark
plazas, park interiors, ceremonial axes and river reservations remain open.
This stage does not automatically fill those areas.

Added: BackgroundResidential.cpp/.h, BackgroundResidentialPlan.h,
MediumResidenceGeometry.h and this document.
Modified: CityResidentialDistricts.cpp (shared helper extraction only),
Scene.cpp/.h and Astana3DTour.vcxproj/.filters.
Validation outputs are ignored under obj/background-residential-check.
Stopped for F5 visual/performance review. No commit or push.

## Subsequent expansion - Part 5A

The original forty buildings and ten courts described above are preserved.
The same six archetypes now also serve sixty additional buildings in ten
new groups through static X/Z scale and tint instance parameters. No shared
building meshes or previous site buffers change. See
[BackgroundCityExpansion.md](BackgroundCityExpansion.md) for the new site
plan, counts, validation and matched performance comparison.
