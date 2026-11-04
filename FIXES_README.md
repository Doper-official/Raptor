# Raptor — Y-up, GLB, lighting and directional-shadow fixes

This archive is based on the supplied `holly compressor.zip` and the earlier Y-up/GLB fixes, with a new directional shadow-map implementation added.

## Main fixes

### 1. Y-up camera convention
- Perspective projection flips clip-space Y for Raptor's positive-height Vulkan viewport. World coordinates remain Y-up.
- `setViewDirection()` now builds the same right/up/forward basis as `setViewYXZ()`.
- Default camera up is +Y.
- E/Q vertical movement and mouse pitch use the same Y-up convention.
- The orthographic projection's existing Vulkan Y handling is retained.

### 2. GLB transforms and normals
- The importer walks the active glTF scene graph, including parent/child transforms.
- Node translation, rotation, scale, and matrix transforms are applied to positions.
- Normals use the inverse-transpose transform and are normalized.
- Mirrored transforms reverse triangle winding.
- Missing/zero normals get generated from transformed triangles; valid supplied normals remain intact.
- The supplied GLB normals were audited separately; see `MODEL_NORMAL_AUDIT.md`.

### 3. Demo scene placement
- The vase's GLB node rotation is applied by the importer and cancelled by the demo object's transform so the vase stands along Raptor's Y axis.
- The floor is at approximately Y = -0.402, directly below the vase's lowest vertex (approximately -0.4015 after the intended transform).

### 4. Directional shadow mapping — newly implemented
- Added `DirectionalShadowSystem`.
- Added one 2048×2048 depth map per frame in flight, avoiding a shared depth-image write/read race between overlapping frames.
- Added a depth-only render pass, framebuffer, and graphics pipeline.
- Added `shadow_map.vert` and `shadow_map.frag`.
- Added a light view-projection matrix to `GlobalUbo`; all GLSL copies of the UBO use the same member order.
- Added C++ `static_assert`s for important std140 offsets, `PointLight` array stride, and total UBO size.
- The shadow pass draws all opaque model primitives with the same object transform as the geometry pass.
- The deferred pass samples the shadow depth image and applies 3×3 PCF to directional diffuse and specular lighting.
- Uses slope-aware shader bias and rasterizer depth bias to reduce shadow acne.
- Shadow-map sampling clamps to an opaque-white border, so taps outside the light frustum are treated as lit rather than wrapping.

### 5. Vulkan resource and pipeline wiring
- `RaptorPipelineConfigInfo` now supports a true zero-color-attachment pipeline for depth-only rendering.
- The deferred descriptor set has a fourth binding for the shadow map, and the descriptor pool allocation matches it.
- Shadow render-pass dependencies cover early/late depth tests and fragment sampling.
- Depth images use nearest filtering and clamp-to-border sampling.
- CMake registers the new shadow-system source. The existing shader batch script already compiles all `.vert` and `.frag` files, so it includes the new shadow shaders.
- Restored the missing `copyModels.bat` required by the supplied CMake configuration.

## Direction convention

The UBO stores the direction the light rays travel. For a sun above a Y-up scene, a ray vector such as `(-0.5, -1.0, -0.3)` travels downward. The deferred shader negates that vector to get the surface-to-light direction, which has positive Y.

The shadow light camera uses `lookAtLH` and `orthoLH_ZO`, matching Raptor's +Z-forward camera convention. This avoids mixing a right-handed GLM light camera with the engine's left-handed-style camera/projection.

## What is still not implemented

- **Point-light shadows:** the existing point lights remain supported for illumination, but need cube-map/six-face shadow maps to cast shadows.
- **Alpha-tested shadows:** the depth pass currently treats all triangles as opaque. Cutout textures (foliage, grates) need an alpha test and texture descriptor in the shadow pass.
- **Cascaded shadow maps:** the directional light uses one fixed orthographic volume (±5.5 world units around the demo scene, near 0.1, far 28). Large/streaming worlds need fitted or cascaded maps.
- **PCSS/area-light shadows:** current 3×3 PCF softens edges modestly; it is not physically based area-light shadowing.

## Verification status

- The ZIP contents and source wiring were checked after patching.
- The GLB NORMAL/POSITION/index accessors were audited directly (see `MODEL_NORMAL_AUDIT.md`).
- C++ brace balance, descriptor binding counts, shader UBO declarations, and CMake source registration were checked.
- This environment does not have Vulkan headers, the Vulkan SDK, `glslc`, or `glslangValidator`, so I could not compile the C++ project or generate SPIR-V here. Run the normal Windows CMake build with your Vulkan SDK and validation layers enabled. Runtime shadow tuning (bias/frustum/PCF) still needs a visual test on your GPU.
