# G9C — camera-local lawn grass

Opt-in detail for the Botanical Garden's eight existing central grass sectors
and the actual lawn subdivisions of the five CityResidentialDistricts courtyards.
No terrain, paths, planting sites or existing visible meshes were changed.
Other lawns deliberately remain unclassified and receive no detail geometry.

Placement uses existing lawn material triangles. Projected near-ground mesh
triangles exclude paving, flowerbeds, planting and furniture; tree roots also
have a 0.7 m exclusion disc. Candidate roots require lawn support in four
directions and retain 0.18 m clearance from exclusion geometry. Courtyard lawn
subdivision already removes building foundations, entrance aprons and paths.
No city-wide grass-colored-pixel test or assumed parcel rectangle is used.

The shared opaque mesh has five bent, tapered ribbon blades, 15 triangles and
three deterministic profile variations. Typical height is 8–17 cm. Instances
vary yaw, tint, profile and height on a jittered 0.4 m lattice. Both blade sides
render without alpha sorting. There is no wind animation or detailed shadow
casting; blades receive the existing daylight and directional shadows.

All 61,652 candidate tufts are uploaded once into 315 cached 8 m cells over six
sites, using approximately 1.88 MiB of vertex/index/instance buffers. Only local,
frustum-visible cells draw. Full density reaches 9 m, then stable per-root
thresholds smoothly shrink and remove tufts by 23 m. Height fading starts at
3.5 m and finishes at 8 m; high aerial views submit no grass. Walking, sprinting
and teleporting reuse static buffers without cell regeneration or GPU uploads.

Debug x64 build, mesh normals/winding, shader compilation and OpenGL checks
passed. Matched Botanical Garden, courtyard, path-edge and elevated captures
were inspected. Shifted walking viewpoints, low/high flight, teleportation and
F6/F7/F8 combinations passed. Elevated grass ON/OFF images were pixel-identical.
Tested walking views added 9–15 cell draws and 21,315–40,080 submitted triangles;
figures vary with the camera. No extra fullscreen pass or shadow caster draws.

Brief matched Botanical Garden test: Intel UHD, Debug 1280×720, F6/F7/F8 enabled,
grass OFF 67.95 FPS / 14.72 ms; ON 68.91 FPS / 14.51 ms. The small difference is
measurement noise, not an FPS improvement claim. No material regression was
observed in this short sample; it is not a broad performance benchmark.

Changed: `GrassDetail.cpp/.h`, lawn-mask capture/accessors in
`BotanicalGarden.cpp/.h` and `CityResidentialDistricts.cpp/.h`,
`Scene.cpp/.h`, Visual Studio project/filter references and this document.
Ground/water shaders, G9A/G9B models, shadow maps and controls are untouched.
Remaining limits: selected lawns only, fixed low-poly blades, no wind, and no
coverage for arbitrary terrain or every landscaping module. No commit or push.
