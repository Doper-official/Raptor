# Raptor shadow + Y-up patch

## Added in this patch

- A depth-only directional shadow pass (`DirectionalShadowSystem`).
- One 2048x2048 depth map per frame in flight, avoiding concurrent frame read/write hazards.
- A light view-projection matrix stored in `GlobalUbo` and declared in the same std140 order in every shader that reads that UBO.
- A depth-only Vulkan pipeline, depth render pass, framebuffers, depth image sampling, and explicit render-pass dependencies.
- 3x3 percentage-closer filtering (PCF) in the deferred lighting shader.
- Slope-aware shader bias plus rasterizer depth bias to reduce shadow acne.
- Directional diffuse and specular light are shadowed. Point lights remain supported but are not shadowed by this directional map.
- Shadow-map images use clamp-to-white border sampling so taps outside the light map count as lit, not as wrapped geometry.

## Coordinate convention

Raptor uses Y-up and +Z-forward. The directional-light vector in the UBO is the direction the light rays travel. For a sun above the scene it has negative Y, e.g. `(-0.5, -1.0, -0.3)`. The deferred shader negates it to obtain the vector from the surface toward the light.

The light-camera matrix uses `lookAtLH` + `orthoLH_ZO`, matching Raptor's +Z-forward view/projection convention. The shadow texture UVs are derived from the same Vulkan NDC-to-framebuffer convention used when rendering the depth map.

## Things this does not do yet

- No point-light shadows (those need cube-map or six-face shadow maps).
- No alpha-tested/cutout shadowing. The current shadow pass treats model triangles as opaque, so a future foliage/fence material needs its base-color alpha texture tested in the shadow fragment shader.
- No cascaded shadow maps. The directional shadow camera uses a fixed orthographic volume centered on the demo scene (`±5.5` units, near `0.1`, far `28`). Expand or fit these bounds for larger worlds.
- No PCSS/soft-area shadows. The current 3x3 PCF is a modest filtered hard-shadow approximation.

## Re-analysis notes

- GLB model transforms are applied by the patched loader. Source normals in the supplied GLBs were finite, normalized, and aligned with their source triangle winding; the loader keeps valid normals and repairs missing/zero ones.
- The plane asset's normals point +Y, so the plane is oriented upward in the Y-up scene.
- The earlier issue where the vase and plane were vertically separated was addressed in the prior patch. The shadow map now uses the same object model matrices as the geometry pass.
- The geometry pass depth is still the camera depth buffer; the new shadow depth image is a separate light-space depth attachment.

## Build

Run the usual Windows CMake build. `shaders/compileShaders.bat` compiles every `.vert` and `.frag` file, including the new `shadow_map.vert` and `shadow_map.frag`.

This archive has been source-reviewed and statically checked in this environment. It has **not** been built or run against your Windows Vulkan SDK/GPU here, so validation-layer and visual tuning on your PC are still required. If your GPU reports no supported depth format with both `DEPTH_STENCIL_ATTACHMENT` and `SAMPLED_IMAGE` features, the shadow image format selection will fail explicitly rather than silently running without shadows.
