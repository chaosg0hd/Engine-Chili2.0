# Current Light Path Alignment

This document maps the current real point-light path to the engine's Direct Light Evaluation Contract.

Direct-light law:

`L_o(P, V) += f_r(P, V, Wi) * L_i(P, Wi) * max(0, dot(N, Wi)) * Vis(P, Wi)`

This is not a future design note. It is a read of the current implemented path.

## Scope

Current vertical slice only:

- source: point / omni light
- path: direct
- visibility: raster cubemap visibility path
- interaction: direct BRDF only
- realization: real-time dynamic DX11 path

## Inputs

Current source inputs:

- `LightPrototype` authored in [light.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/entity/appearance/light.hpp:75)
- lowered into `RenderDirectLightData` in [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:170)
- compiled from authored view lights in [render_frame_compiler.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/render_frame_compiler.cpp:133)

Current surface/material inputs:

- `MaterialPrototype` and `MaterialBrdfPrototype` in [material.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/entity/appearance/material.hpp:10)
- lowered into `RenderMaterialData` in [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:75)
- compiled from object/material prototypes in [object_render_compiler.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/object_render_compiler.cpp:11)

Current camera/view inputs:

- `CameraPrototype` lowered into `RenderCameraData` in [render_frame_compiler.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/render_frame_compiler.cpp:50)
- consumed by the DX11 pixel shader through `cameraPosition` and `cameraExposure` in [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:82)

## Stage A — Source Sample

This stage computes:

- `Wi`
- `d`
- unoccluded `Li`

Current implementation:

- `DirectLightSample` is declared in [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:177)
- point-light sampling is implemented in `SamplePointLight(...)` at [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:198)

Current term mapping:

- `Wi`
  - `sample.incidentDirection = toLight / sample.pathLength`
- `d`
  - `sample.pathLength = max(length(toLight), 0.0001f)`
- unoccluded `Li`
  - attenuation from `directLightSourcePositionRange[lightIndex].w`
  - `sample.incidentRadiance = directLightSourceRadiance[lightIndex].rgb * attenuation`

Current upstream binding:

- direct-light source data is packed in `DrawObject(...)` at [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:2082)
- source position/range:
  - `constants.directLightSourcePositionRange[...]`
  - [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:2165)
- source radiance:
  - `constants.directLightSourceRadiance[...]`
  - [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:2169)

## Stage B — Visibility

This stage computes:

- `Vis`

Current implementation:

- visibility test helper: `EvaluateShadowVisibility(...)` at [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:191)
- called from `SamplePointLight(...)` at [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:217)

Current term mapping:

- `Vis = 1.0`
  - when visibility is disabled for the sampled path
- cubemap visibility scalar
  - produced by comparing normalized light distance against sampled cubemap depth
- stored as:
  - `sample.visibility`

Current upstream visibility inputs:

- visibility payload compiled into `RenderDirectLightVisibilityData` in [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:162)
- visibility policy lowered in [render_frame_compiler.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/render_frame_compiler.cpp:63)
- direct-light visibility pass selected in [render_frame_compiler.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/render_frame_compiler.cpp:326)
- DX11 direct-light visibility realization run in [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:1775)

## Stage C — BRDF Interaction

This stage computes:

- direct BRDF response from `N`, `V`, `Wi`, and material descriptors

Current implementation:

- surface inputs are assembled into `SurfaceShadingContext` in [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:161)
- BRDF evaluation is implemented in `EvaluateDirectBrdf(...)` at [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:227)

Current term mapping:

- `N`
  - `surface.normal`
- `V`
  - `surface.viewDirection`
- `Wi`
  - passed in as `incidentDirection`
- material descriptors
  - `surface.surfaceAlbedo`
  - `surface.reflectionColor`
  - `surface.diffuseStrength`
  - `surface.specularStrength`
  - `surface.specularPower`
  - `surface.reflectivity`
  - `surface.roughness`

Current material lowering:

- material interaction data compiled in [object_render_compiler.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/object_render_compiler.cpp:53)
- direct BRDF payload stored in `RenderMaterialData::directBrdf` in [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:77)

## Stage D — Contribution Assembly

This stage computes:

- `Contribution = f_r * Li * NdotL * Vis`

Current implementation:

- implemented in `AssembleDirectLightContribution(...)` at [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:241)

Current term mapping:

- `f_r`
  - `brdfResponse = EvaluateDirectBrdf(...)`
- `Li`
  - `sample.incidentRadiance`
- `NdotL`
  - `sample.ndotl`
- `Vis`
  - `sample.visibility`
- final contribution
  - `contribution.radiance = brdfResponse * sample.incidentRadiance * sample.ndotl * sample.visibility`

## Stage E — Accumulation

This stage sums direct-light contributions.

Current implementation:

- per-light accumulation happens in `PSMain(...)` at [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:257)
- loop body:
  - `DirectLightSample lightSample = SamplePointLight(...)`
  - `DirectLightContribution contribution = AssembleDirectLightContribution(...)`
  - `litColor += contribution.radiance`
  - [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:311)

## Transitional Non-Direct Term

The current path still carries one explicit non-direct term:

- ambient contribution:
  - `litColor = surfaceAlbedo * nonDirectAmbientColor.rgb * ambientStrength`
  - [dx11_render_backend.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/backend/dx11_render_backend.cpp:288)

This is intentionally outside the direct-light law and should be treated as transitional non-direct lighting.

## Secondary Lighting Direction

The next non-direct lighting step should not be documented as one monolithic "indirect lighting" blob.

The intended secondary-lighting family is:

- derived bounce fill
- probe-based indirect lighting
- screen-space indirect lighting

These should be treated as secondary-light providers that eventually feed a shared indirect accumulation stage.

Conceptual future model:

`Indirect = BaseBounce + ProbeIndirect + ScreenRefinement`

This document still covers only the current direct-light path plus the current transitional ambient term.

## Current Alignment Summary

The active point-light slice now maps to the contract in this order:

1. source sample
   - `SamplePointLight(...)`
2. visibility
   - `EvaluateShadowVisibility(...)`
3. BRDF interaction
   - `EvaluateDirectBrdf(...)`
4. contribution assembly
   - `AssembleDirectLightContribution(...)`
5. accumulation
   - `PSMain(...)` loop

## Transitional Areas Still Present

- `RenderShadowPolicyData` remains realization-named even though it now feeds the direct-light visibility lane.
- `RenderPassDataKind::Shadow` and `ViewKind::ShadowCubemapFace` still use old realization naming.
- ambient remains outside the direct-light contract.
- only the point-light + raster cubemap visibility slice is aligned here.
