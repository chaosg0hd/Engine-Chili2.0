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
