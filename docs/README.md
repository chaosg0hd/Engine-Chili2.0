# Engine-Chili2.0 Documentation

This repo now has separate documentation tracks for the engine and the studio.

Docs map:

- [Engine Core Docs](./engine/README.md)
- [Studio Docs](../apps/studio/README.md)
- [CSS Design Guidelines](./css/README.md)
- [Visual Language](./css/visual-language.md)

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - feature-test sandbox app
- `engine_studio` - native studio host that now consumes the engine's web dialog API for CoreTools

Quick summary:

- `engine_core` owns the native runtime, platform window, rendering, input, diagnostics, jobs, memory, files, GPU capability services, and engine-level web dialogs
- `engine_sandbox` is the current feature-test app harness that exercises engine systems directly
- `engine_studio` is the editor-facing native host that embeds a WebView2 CoreTools surface inside the main studio window

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
