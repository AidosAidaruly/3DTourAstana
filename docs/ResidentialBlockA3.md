# Premium Residential Block A3 - City Development Part 3A

## Documented site and placement

The selected plot is A3 from SecondaryStreetPlan and ResidentialDistrictPlan:
**X=-659..-496, Z=-859..-747**, **163 x 112 m**, anchor **(-577.5,0,-803)**.
It is the western of the two documented 163 x 112 m plots, closer to EXPO
than A4. It lies south of Astana Arena, east of MEGA and wholly inside the
existing terrain. The road system and all other parcels are preserved.

All building bodies, facade relief, aprons and courtyard elements fit A3.
Only three short pedestrian verge links extend beyond its edges to existing
kerbs; they occupy roadside reservations, never another development plot.
There are no buildings, parking lots or visible plot markers elsewhere.

| Building | World center X/Y/Z | Body footprint X/Z | Total floors | Maximum height |
| --- | --- | --- | ---: | ---: |
| A | -629.5 / 0 / -798 | 28 x 30 m | 16 | 56.25 m |
| B | -525.5 / 0 / -796 | 30 x 32 m | 22 | 76.65 m |
| C | -577.5 / 0 / -837 | 40 x 28 m | 28 | 97.10 m |

Floor counts include one 4.2 m ground level and residential storeys at 3.35 m.
Foundations top out at Y=0.18. Roof slab elevations are 54.63 / 74.73 / 94.83 m;
refined crowns bring the final heights to the values above. Two setbacks
occur in each building: the last four floors inset 1.4 / 1.6 / 1.8 m, and the
last two add 0.6 / 0.8 / 1.0 m. Approximate minimum structural corner separation
is 20.25 m; even a conservative envelope including facade relief leaves more
than 18 m. The main courtyard opens north/+Z with about 75 m between the two
side buildings' inner facade planes.

Body-to-carriageway clearance is at least 23 m on the nearest western,
eastern and southern streets before accounting for their separate aprons.
No building, foundation, canopy or landscaped element occupies a street,
intersection, existing parking lot, promenade or completed landmark.

## Architecture and entrances

PremiumResidenceGeometry.h adapts the original EXPO Premium Residence
techniques into one parameterized generator shared by all three new buildings.
The completed ExpoResidence and ExpoPremiumResidence files are unchanged.

- Ivory/light limestone bases, corner piers and vertically grouped fins.
- Coherent warm brown brick zones with the existing procedural mortar shader.
- Real facade apertures, deep stone reveals and opaque reflective blue-gray
  glass; no large solid backing cube behind decorative window overlays.
- 1,960 glazed openings including 88 curved ground-floor arches.
- 446 recessed loggias, approximately 1.45 m deep, with floor edges, horizontal
  metal rails and fine vertical balusters. Paired living bays project 0.48 m;
  their side returns and upper/lower caps close each transition.
- Continuous restrained floor bands, full four-sided elevations at every
  floor, stepped upper levels, closed terrace decks and attached guardrails.
- Finished parapets, silver/ivory crown framing and small capped roof equipment.

Five entrances have recessed double-leaf glazing, metal framing, handles,
stone portals and modest canopies. Three face the shared courtyard; the two
side buildings also have street-facing entrances. Facade-plane centers:

| Entrance | World X/Y/Z | Facing |
| --- | --- | --- |
| A courtyard | -615.5 / 0.18 / -798 | +X |
| A street | -643.5 / 0.18 / -798 | -X |
| B courtyard | -540.5 / 0.18 / -796 | -X |
| B street | -510.5 / 0.18 / -796 | +X |
| C courtyard | -577.5 / 0.18 / -823 | +Z |

Glazing is recessed a further 0.58 m behind these facade planes. Three-metre
aprons grade gently from the door/foundation level Y=0.18 to Y=0.06 paving,
about a 4 percent rise, without entrance steps or a raised site platform.

## Courtyard and public access

A connected six-metre central approach, perimeter walks and lateral branches
serve all three courtyard entrances. The design includes four lawn islands,
four low stone-bordered planting beds, 16 shrubs, eight grass clusters,
four small deciduous trees and six wood-and-metal benches. The central axis
is unobstructed. Most of the remaining parcel stays natural grass for future
landscaping and site design. No large rectangular terrain overlay is added.

Three street-edge receiving points are derived from the actual secondary
road widths and 20 cm kerbs:

- West: X=-669.8, Z=-800..-796, at the east edge of S1's kerb.
- East: X=-484.7, Z=-798..-794, at the west edge of S7's kerb.
- North: X=-580.5..-574.5, Z=-734.7, at the south edge of S4's kerb.

Their final two metres rise from Y=0.06 to the existing kerb top Y=0.14.
Paving terminates at the kerb outside edges, with no new road cuts, kerb
removal or asphalt overlap. Detailed roadside sidewalks/crossings remain
reserved for the later public-realm stage; these paths are not new vehicle
accesses. Total added pedestrian paving is approximately 3,014.2 square metres,
distributed around the buildings and courtyard rather than covering the plot.

## Static batches, LOD and cost

All geometry is generated once and uploaded into one VAO/VBO/EBO. Twelve
base opaque material ranges batch all three buildings and their grounds.
Two additional metal-detail ranges provide window/mullion profiles and fine
balusters/handles. There are no per-window, per-floor or per-building draws,
no new transparency sorting and no per-frame geometry or instance uploads.
Conventional static batching is sufficient for this one block; a new instanced
facade renderer would add machinery without reducing its small draw count.

| View detail | Submitted triangles | Draws |
| --- | ---: | ---: |
| Nearby, LOD0 | 123,812 | 14 |
| Medium, LOD1 | 118,340 | 13 |
| Far, LOD2 | 92,516 | 12 |
| Outside block frustum | 0 | 0 |

The full architecture cost per building is 28,852 / 37,738 / 51,754 triangles.
The remaining 5,468 triangles are immediate site/courtyard geometry. The
complete static vertex/index allocation is 9,314,224 bytes, about 8.9 MiB.
Far views save about 25.3 percent of the near submitted triangles while
retaining every opening, loggia, floor band and the stepped silhouette.

LOD uses camera-to-building-box distance and stable hysteresis: fine detail
leaves at 205 m and returns below 155 m; frame detail leaves beyond 500 m
and returns below 420 m. Teleports update the selection immediately. No
blended LOD or transparency pass is required for subpixel profiles. The
whole block also has conservative frustum culling including its access paths.

Existing opaque glass, brick, grass and paving shader capabilities are reused
without shader edits. Depth/blend settings are retained, and temporary paving
polygon bias and material flags are restored. Glass caches, stadium seat
instancing, parked-vehicle LOD/instancing and the FPS/movement display remain
untouched. No measured FPS claim is made for this stage.

## Validation and scope

Debug x64 builds successfully without new C++ warnings; existing GLFW
missing-PDB linker warnings remain. The overview, courtyard, entrance,
medium-distance and far-distance views were inspected. All three LODs and
an off-screen rejection were exercised. Shader compilation, GL errors and
depth/blend/material-state restoration passed in the full scene.

Uploaded mesh checks pass for finite positions/normals/UVs, valid indices,
unit normals, winding and nondegenerate triangles. Paving has no overlapping
areas, and none of the three connections overlaps existing carriageways or
kerb tops. Roof height assertions now check every building; the visual check
caught and corrected double-counted terrace railing elevation before final
validation, so no floating railings remain above the crowns.

Both original EXPO residences, all roads, existing landmarks, shaders,
camera/teleport code and unrelated working-tree files retain their starting
hashes. A3 alone is allocated in the planning document.

Limitations: glazing is intentionally opaque, doors are static, no apartment
interiors or new parking are modeled, and farther LODs omit subpixel profiles.
The exterior and restrained courtyard are ready for the user's F5 approval.
No additional residential parcel is populated. No commit or push.

Changed: src/CityPremiumResidence.cpp/.h, src/PremiumResidenceGeometry.h,
src/Scene.cpp/.h, Astana3DTour.vcxproj/.filters,
docs/ResidentialDistrictPlan.md and docs/ResidentialBlockA3.md.
