# Compiled Render-Light Contract

This document defines the render-owned compiled light contract.

It is the narrow bridge between:

- prototype light meaning
- runtime direct-light computation law
- backend / shader execution

It is not another prototype family.
It is not a backend-specific struct.
It is the render module's private, executable light description.

## Purpose

The compiled render-light contract exists to answer:

- what light data runtime shading actually needs
- what visibility data the frame needs
- what pass work must exist before main shading
- what authored light meaning can be ignored after lowering

It should let the render module lower:

`LightPrototype -> compiled shading-facing light data + compiled light work`

without dragging authoring semantics or backend object ownership into runtime shading.

## Position In The Sequence

This contract comes after:

- Direct Light Evaluation Contract
- Current Light Path Alignment

because the current runtime law now tells us exactly which terms the renderer must carry:

- source sample terms
- visibility terms
- BRDF-facing light inputs
- contribution/accumulation dependencies

Before that point, compiled light data would be guesswork.

## Inputs

The compiled render-light contract is lowered from public prototype-side data, but keeps only the runtime-executable subset.

Current prototype-side inputs:

- `LightPrototype`
  - [light.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/entity/appearance/light.hpp:75)
- view/camera context
  - [view.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/presentation/view.hpp:18)
- current visibility policy
  - [light_visibility.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/entity/appearance/light_visibility.hpp:8)

Lowering is currently performed in:

- [render_frame_compiler.cpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/render_frame_compiler.cpp:133)

## Outputs

The compiled contract should produce two render-owned outputs.

### A. Compiled light shading data

This is what the main lighting pass consumes.

It must be enough to support:

- `Wi`
- `d`
- unoccluded `Li`
- visibility lookup inputs
- light participation in current direct-light evaluation

### B. Compiled light work data

This is what pass scheduling / frame preparation consumes.

It must answer:

- does this light require pre-shading work
- what work must exist before the main scene pass
- how many views/passes are required
- whether refresh is required or optional this frame

This split prevents one giant mixed blob of "light plus pass plus backend plus authoring."

## Required Shape

Conceptually, the compiled contract should be split into two render-owned pieces.

### `CompiledLightData`

The shading-facing compiled record.

Mandatory fields should cover:

- source type/class
- enabled flag
- source position if relevant
- source direction if relevant
- source color
- source intensity
- source range if relevant
- cone values if relevant
- visibility mode
- visibility lookup slot/reference index
- visibility bias values
- visibility near/far values if required
- direct-light participation flags

### `CompiledLightWork`

The pass/work-facing compiled record.

Mandatory fields should cover:

- whether a visibility pass is required
- what visibility pass class/type is needed
- how many views are required
- whether refresh is required this frame
- whether refresh is optional / budgeted

## First Supported Path

The first compiled path that must be supported well is:

- source: point / omni
- path: direct
- visibility: raster cubemap
- interaction: direct BRDF only
- realization: real-time dynamic

This path implies:

- shading needs point-light sample data
- visibility needs cubemap lookup inputs
- frame prep needs a 6-face visibility pass before the scene pass

The contract should be shaped so directional and spot can fit later, but it only needs to serve this path correctly first.

## Mandatory Runtime Fields

For the current first supported path, the compiled contract must be able to carry:

### Source sampling data

- light type
- enabled
- position
- color
- intensity
- range

These fields must be sufficient to compute:

- `Wi`
- `d`
- attenuation
- unoccluded `Li`

### Visibility data

- visibility mode
- active visibility resource slot/index
- depth bias
- near plane
- far plane
- visibility enabled state

These fields must be sufficient to compute:

- `Vis(P, Wi)`

### Interaction gating

- affects direct lighting
- any future direct diffuse/specular participation flags if needed

These fields are not BRDF internals.
They only say whether the light participates in the current direct-light evaluation path.

### Realization/runtime policy

- dynamic/static-for-frame meaning if needed
- quality tier / budget class if needed
- update needed this frame
- refresh required/optional

These fields must be frame-relevant only.

## Forbidden Contents

The compiled render-light contract must not contain:

- authoring-only explanations or semantic commentary
- backend raw objects
  - texture pointers
  - SRVs
  - DSVs
  - device handles
- sandbox/debug UI state
- prototype composition hierarchy
- file/resource import details
- BRDF internals
- app-facing strings and IDs unless strictly needed for debug tracking

This keeps the contract lean, render-owned, and executable.

## Current Approximation In Code

The current runtime path is only an approximation of this intended contract.

Today it is represented by:

- `RenderDirectLightData`
  - [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:170)
- `RenderDirectLightSourceData`
  - [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:152)
- `RenderDirectLightVisibilityData`
  - [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:162)
- pass metadata inside `RenderPassData` / `RenderViewData`
  - [render_frame_data.hpp](/C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/render/render_frame_data.hpp:195)

This is enough for the current aligned point-light path, but it is still transitional because:

- light work is not yet split into its own explicit compiled record
- realization-specific naming still leaks through
- visibility/pass work is still carried alongside general frame data rather than through a dedicated compiled light work shape

## Next Implementation Target

The next code step after this spec should be:

- introduce explicit render-owned compiled light records
  - `CompiledLightData`
  - `CompiledLightWork`
- lower from `LightPrototype` into those records inside the render module/compiler path
- let frame/pass preparation consume `CompiledLightWork`
- let shading consume `CompiledLightData`

That is the next clean runtime-facing step.

## Secondary Lighting Boundary Note

This document is about the compiled direct-light contract.

Future secondary-light providers such as:

- derived bounce fill
- probe-based indirect lighting
- screen-space indirect lighting

should not be forced into this compiled direct-light shape just because they also affect lighting.

They should eventually reduce to their own render-owned indirect contribution/provider contracts and then feed a shared indirect accumulation stage alongside, not inside, the direct-light compiled path.
