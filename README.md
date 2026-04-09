# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - active sandbox app under `apps/sandbox/`
- `engine_studio` - native studio host with an embedded WebView2 CoreTools surface

Current state:

- `engine_sandbox` now owns the active sandbox harness under `apps/sandbox/src/`
- the older broad sandbox harness is preserved under `apps/sandbox/archive/`
- frame submission now flows through `RenderFramePrototype` from app-facing code into the renderer/GPU path
- the DX11 backend currently clears and presents, but does not yet realize submitted frame contents into visible geometry
- in-window overlay text is currently a separate Win32 paint path, not renderer-owned text

Architecture snapshot:

- `PlatformModule` owns the OS window and render surface
- `GpuModule` owns the backend, device-facing presentation path, and generic GPU resources
- `RenderModule` owns render submission flow and frame orchestration
- `ResourceModule` owns engine-facing asset/resource state
- `JobModule` owns worker execution
- `MemoryModule` owns allocation policy and tracking

Render path snapshot:

- sandbox/app code builds a `RenderFramePrototype`
- `EngineCore` forwards it to `RenderModule`
- `RenderModule` submits it through `GpuModule`
- the DX11 backend currently clears and presents that frame, but does not yet draw frame items

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

- the active sandbox is currently a focused parallel frame-preparation test
- it stress-tests CPU-side frame/pass generation with the job system
- it is useful for engine wiring, pacing, and frame-flow validation
- it is not yet a proof of real DX11 geometry rendering

See [docs/README.md](docs/README.md) for the current architecture, feature list, and API inventory.
