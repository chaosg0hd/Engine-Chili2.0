# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - active sandbox app under `apps/sandbox/`
- `engine_studio` - native studio host with an embedded WebView2 CoreTools surface

Current state:

- `engine_sandbox` is now a thin harness under `apps/sandbox/src/`
- the active progressive-hex scheduler now lives in engine-side controller/strategy/compiler code
- the reusable moving-cube faux scene sampler now lives under `src/prototypes/systems/`
- older one-off sandbox variants have been stripped down; `hex_observation` remains in archive while its debug types/logic migrate into `src/modules/diagnostics/`
- frame submission now flows through `FramePrototype` from app-facing code into the renderer/GPU path
- prototype families now live under:
  - `src/prototypes/presentation/`
  - `src/prototypes/entity/appearance/`
  - `src/prototypes/entity/geometry/`
  - `src/prototypes/entity/object/`
  - `src/prototypes/entity/scene/`
  - `src/prototypes/math/`
- `RenderModule` now translates prototype requests into render-owned `RenderFrameData`
- `GpuModule` and backend internals now consume render-owned data instead of prototype structs directly
- the DX11 backend now realizes submitted frame contents into visible geometry
- `ViewPrototype` can now carry light-ray prototypes into render-owned frame data
- in-window overlay text is currently a separate Win32 paint path, not renderer-owned text

Architecture snapshot:

- `PlatformModule` owns the OS window and render surface
- `GpuModule` owns the backend, device-facing presentation path, and generic GPU resources
- `RenderModule` owns render submission flow, prototype-to-render translation, and frame orchestration
- `ResourceModule` owns engine-facing asset/resource state
- `JobModule` owns worker execution
- `MemoryModule` owns allocation policy and tracking

Render path snapshot:

- sandbox/app code builds a `FramePrototype`
- `EngineCore` forwards it to `RenderModule`
- `RenderModule` translates it into render-owned `RenderFrameData`
- `RenderModule` submits that through `GpuModule`
- the DX11 backend uploads and draws submitted frame items from render-owned data

Run instructions:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja
cmake --build build
```

Codex note:

- In this repo, direct CMake commands are the reliable path.
- Codex may need to run `cmake -S . -B build -G Ninja` and `cmake --build build` outside the sandbox because CMake try-compile/build subprocesses can stall or fail under sandboxed execution.

Sanitizer build:

```powershell
Remove-Item -Recurse -Force build\sanitize -ErrorAction SilentlyContinue
cmake -S . -B build\sanitize -G Ninja -DENABLE_SANITIZERS=ON
cmake --build build\sanitize
```

Studio status:

- `engine_studio` keeps the existing native engine window and hosts an embedded WebView2 sidebar
- The first embedded tool surface lives under `apps/studio/coretools`
- The current milestone is Windows-only and focused on a fixed left-docked CoreTools surface
- Future HTTP and WebSocket transport code remains under `apps/studio/src/transport`, but it is not the active runtime path for this milestone

Sandbox status:

- the active sandbox is currently a progressive hex render-priority algorithm lab
- the sandbox app now mainly configures `ProgressiveHexRenderController`, wires `MovingCubeSampleScenePrototype`, and writes debug logs
- visualization is currently the pre-DX stable path: native progressive hex patches plus screen patches, not DX-owned region rerendering
- the current sandbox scene is a CPU-sampled faux render target with moving rotating cubes
- recursive setup maps occupied screen regions to center-pass chains and runtime scheduling refreshes deeper regions more often without clumping repeated passes together
- center passes push placeholder values to constituent descendant cells, while deeper passes refine those placeholders through temporal blending
- generic `ScreenPatch` and `ScreenHexPatch` renderer support remains the current screen-space debug/output lane
- next renderer-facing milestone is still to expose the scheduler as renderer-owned work/update jobs instead of treating sampled colors as the final algorithm output

See [docs/README.md](docs/README.md) for the current architecture, feature list, and API inventory.
