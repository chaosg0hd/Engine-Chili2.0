# Engine-Chili2.0 Documentation

This repo now has separate documentation tracks for the engine and the studio.

Docs map:

- [Engine Core Docs](./engine/README.md)
- [Future Engine Shape](./engine/future-shape.md)
- [Studio Docs](../apps/studio/README.md)
- [CSS Design Guidelines](./css/README.md)
- [Visual Language](./css/visual-language.md)

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - feature-test sandbox app
- `engine_studio` - native studio host that now consumes the engine's web dialog API for CoreTools

Quick summary:

- `engine_core` owns the native runtime, platform window, rendering, input, diagnostics, jobs, memory, files, GPU capability services, and engine-level web dialogs
- `engine_sandbox` is the current prototype-driven renderer bring-up app under `apps/sandbox/`
- `engine_studio` is the editor-facing native host that embeds a WebView2 CoreTools surface inside the main studio window
- prototype families now live under:
  - `src/prototypes/presentation/`
  - `src/prototypes/entity/appearance/`
  - `src/prototypes/entity/geometry/`
  - `src/prototypes/entity/object/`
  - `src/prototypes/entity/scene/`
  - `src/prototypes/math/`
- `RenderModule` consumes `FramePrototype` at its boundary and translates it into render-owned internal data before GPU/backend execution
- the active sandbox is now a progressive hex render-priority algorithm lab using engine-side controller/strategy/compiler pieces and generic screen-space patch submission
- the sandbox currently proves a recursive screen-region scheduling model:
  - setup maps occupied screen cells into chainable center-pass paths, for example `A|K|G|D|G|A`
  - runtime applies parent placeholder passes to descendant cells and lets deeper passes refine them
  - repeated deep-priority passes are distributed through the runtime loop instead of clumped
  - the active faux scene contains moving rotating cubes sampled through the scheduler
- the sandbox harness itself is now thinner:
  - `ProgressiveHexRenderController` owns strategy lifecycle, adaptive budget, overlay text, and debug-log bundle generation
  - `MovingCubeSampleScenePrototype` owns the demo sample scene
- `scenario_shell` has been stripped into reusable scene preset compilers, and `hex_observation` is being mined into diagnostics helpers instead of staying app-owned forever
- next integration target is still a renderer-facing region update job list that can replace the faux color sampler with real render work

Build instructions:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja
cmake --build build
```

Codex note:

- Prefer direct CMake commands over wrapper scripts.
- If Codex sees configure stall around compiler ABI detection or nested try-compile, treat sandbox restrictions as a likely cause and retry with unsandboxed execution when appropriate.

Sanitizer build:

```powershell
Remove-Item -Recurse -Force build\sanitize -ErrorAction SilentlyContinue
cmake -S . -B build\sanitize -G Ninja -DENABLE_SANITIZERS=ON
cmake --build build\sanitize
```
