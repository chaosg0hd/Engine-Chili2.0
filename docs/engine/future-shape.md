# Future Engine Shape

This note captures the intended direction for the engine as it moves from subsystem bring-up toward a more authored, tool-driven runtime.

The short version:

- prototypes describe passive data and intent
- systems own simulation and transformation
- modules own runtime execution and lifetime
- tools author, inspect, and compose engine-facing models

## North Star

Engine-Chili2.0 should become a small native engine core with clear authored data lanes around it.

The engine should not treat rendering as the only center of the world. Rendering is one output lane. Simulation, resources, tooling, diagnostics, and authored spaces should all be able to express intent without being forced through renderer-owned concepts.

The current prototype layer is the early version of that direction:

- `presentation` describes frame, pass, view, and item structure
- `entity` describes scene-facing objects, appearance, geometry, and camera data
- `math` describes shared value types and transforms
- `systems` can describe reusable simulation-facing contracts such as light rays

## Tool Spaces

Tools should grow around abstract spaces rather than one-off panels.

A tool space is an authored working model with:

- a stable identity
- a domain type
- actions
- panels or lanes
- status
- eventual persistence and transport

The first prototype of this idea is `Journal Space` in the Studio shell prototype. It is intentionally simple: a place for design notes, decisions, references, and open signals. The important part is the shape, not the journal feature itself.

Near-term tool-space examples:

- journal space for design notes and decision trails
- scene space for object and prototype composition
- resource space for asset state, imports, and dependency checks
- render space for frame/pass/view inspection
- system space for light rays, diagnostics, and simulation probes

## Prototype Role

Prototypes should stay passive.

They should describe what the app, tool, or system wants to submit, not how the engine executes it. Behavior belongs in systems and modules.

Good prototype responsibilities:

- describe frame structure
- describe object, geometry, material, camera, and light intent
- describe system input and output contracts
- carry debug labels and authoring metadata when needed
- compose into larger descriptions without owning lifetime

Poor prototype responsibilities:

- owning GPU objects
- performing file IO
- driving threads
- mutating global resource state
- hiding renderer or platform behavior inside descriptor methods

## System Role

Systems should become the bridge between authored prototype data and runtime interpretation.

`LightRaySystem` is the first active example. It accepts light-ray emitter contracts, traces them against simple geometry, and produces debug-friendly trace results. The renderer can visualize those results, but the renderer should not own the ray lifecycle.

Future systems can follow the same rule:

- accept passive prototype input
- own per-frame or persistent simulation state
- produce explicit output contracts
- leave rendering, resources, and platform work to their owning modules

## Module Role

Modules should continue to own execution boundaries.

Current intended ownership remains:

- `PlatformModule` owns OS windows, native handles, messages, and render-surface state
- `GpuModule` owns device-facing resources, backend state, presentation, uploads, and future readback paths
- `RenderModule` owns frame orchestration, render queues, pass ordering, and render-private resources
- `ResourceModule` owns asset identity, resource handles, load-state progression, and CPU-side prepared resource data
- `JobModule` owns background worker execution
- `MemoryModule` owns allocation policy, memory classes, tracking, and diagnostics

The future shape should keep shrinking direct concrete reach-through from `EngineCore`. Core can remain the integration shell, but modules should increasingly communicate through explicit service contracts and passive data.

## Authoring Flow

The desired authoring path is:

```text
Tool Space
    |
Prototype / Authored Contract
    |
System or Module Boundary
    |
Runtime-owned Execution State
    |
Diagnostics / Rendered Output / Persisted State
```

Example for the current light-ray direction:

```text
Journal or Scene Tool
    |
LightRayEmitterPrototype
    |
LightRaySystem
    |
LightRayTraceResult
    |
Render debug proxies and diagnostics
```

Example for resource loading:

```text
Resource Tool
    |
Resource request contract
    |
ResourceModule
    |
JobModule file/decode work and GpuModule upload
    |
Ready resource state
```

## Near-Term Shape

The next useful steps are:

- keep the journal-space model in tools small and abstract
- define a shared vocabulary for tool spaces before adding many panels
- keep prototype composition data-only
- move reusable debug geometry toward a non-sandbox home once ownership is clear
- add diagnostics around system outputs, especially light-ray trace counts and hit/miss summaries
- let shader and runtime asset plumbing follow actual debug-proxy needs instead of guessing too early

## Non-Goals For Now

Do not turn this into a universal object model yet.

Avoid these traps while the engine is still taking shape:

- no giant inheritance tree for every authored object
- no single god-prototype that tries to describe all render and system cases
- no renderer ownership of simulation state
- no jobs becoming asset owners
- no tool UI state leaking into runtime module ownership

The engine should grow by small, named contracts that can be inspected, submitted, translated, and owned by the right layer.
