# G9B — low vegetation

Upgraded existing planting without adding planting sites or changing paths,
bed outlines, tree placements, or garden patterns.

- Four shared shrub recipes: compact, spreading, upright and loose clustered
  foliage, with small basal branches and deterministic orientation.
- Clipped hedge segments retain their formal footprint, with rounded shoulders
  and an uneven crown line.
- Five/six-petal flowers have folded petals, crossed stems and small leaves.
  Existing flower colors and planting patterns remain in their original batches.
- Seven-blade ornamental grass tufts use bent, tapered, double-sided opaque
  geometry. No leaf textures, alpha sorting or new shaders.

Connected systems: Botanical Garden outdoor planting, Esil Embankment,
Abu Dhabi landscaping, Khan district shrubs, background residential courtyards,
CityResidentialDistricts shrubs, and the shared Ak Orda approach/parterre meshes.
Specialized greenhouse plants and other independent landmark landscaping remain
unchanged. G9A tree meshes, 570 tree instances and tree shadow proxies are untouched.

## Cost and validation

Recipes are cached once and baked into existing material batches. The ceremonial
shrubs reuse four ranges of the existing Mesh buffer. No additional draw calls,
render passes, textures or GPU resource types. No per-frame plant generation.
Existing group culling is retained; no separate low-plant LOD renderer was added.
The small fixed recipes cost 76/104 triangles per shrub, 72 per hedge segment,
16/18 per flower head, 16 per stem/leaves and 56 per grass tuft.

Initialization recorded 707 baked shrubs, 36 hedge segments, 2,075 flower heads
and stems, and 201 grass tufts: 146,428 recipe triangles including shared assets.
These are mesh-generation counts; reused parterre meshes and ceremonial shrubs
make them different from the total number of rendered world instances.

Debug x64 build passed. Full-scene checks passed finite/unit normals, winding,
shader compilation, OpenGL state, framebuffer resize/reuse, shadow caching and
all eight F6/F7/F8 combinations. Matched screenshots cover the Botanical Garden,
formal parterre, Esil garden, residential shrubs and Abu Dhabi flowerbeds.
Existing placement loops and bed/path geometry were retained.

Brief matched old/new-recipe garden-overview test, Debug 1280×720, Intel UHD,
shadows/color grading/FXAA enabled: 63.20 → 63.29 FPS (15.82 → 15.80 ms).
This short sample indicates comparable performance, not a broad benchmark.
The sampled full frame, including shadow submission, changed from 3,072,993 to
2,994,288 triangles; draw calls stayed at 3,438. Counts vary with the camera.

Remaining limits: fixed low-poly foliage, original planting density, and no new
foliage texture or dedicated low-plant LOD system. No commit or push.
