# Engine-Chili2.0 Documentation

This repo now has separate documentation tracks for the engine and the studio.

Docs map:

- [Engine Core Docs](./engine/README.md)
- [Studio Docs](../apps/studio/README.md)

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - feature-test sandbox app
- `engine_studio` - browser-first studio host scaffold

Quick summary:

- `engine_core` owns the native runtime, platform window, rendering, input, diagnostics, jobs, memory, files, and GPU capability services
- `engine_sandbox` is the current feature-test app harness that exercises engine systems directly
- `engine_studio` is the editor-facing host that keeps backend authority in native C++ while the frontend grows as a web UI

Build helpers:

```powershell
.\clean.cmd
.\configure.cmd
.\build.cmd
.\rebuild.cmd
```

Sanitizer helpers:

```powershell
.\configure.cmd sanitize
.\build.cmd sanitize
```

Or use the CMake presets:

- `sanitize`
- `sanitize-all`
- `sanitize-sandbox`
- `sanitize-studio`
