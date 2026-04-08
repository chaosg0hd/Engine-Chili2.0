# Studio Documentation

## Overview

`engine_studio` is now a native-window-hosted editor shell for Engine-Chili2.0.
The studio keeps the existing native engine window and now creates CoreTools
through the engine's reusable web dialog API.

Current architecture:

```text
engine_studio.exe
|-- native studio window
|-- native engine/main area
`-- left-docked engine-owned web dialog for CoreTools
```

This milestone is Windows-only and intentionally focused:

- the native outer window stays authoritative
- `CoreTools` is the first-party embedded tool surface
- the left dock is fixed-width for now
- transport and command scaffolding remain in the repo, but they are not the runtime center of this milestone
- Studio now proves the engine-level web dialog feature instead of owning its own WebView implementation

## Current Structure

```text
apps/studio
|-- CMakeLists.txt
|-- README.md
|-- config/
|-- coretools/
|   |-- angular/
|   |   |-- README.md
|   |   |-- src/
|   |   |   |-- app/
|   |   |   `-- assets/
|   |   `-- public/
|   |-- runtime/
|   |   |-- index.html
|   |   |-- app.js
|   |   |-- style.css
|   |   `-- panels/
|   |       `-- hello-panel.js
|   `-- README.md
|-- runtime_test/
`-- src/
    |-- main.cpp
    |-- studio_app.hpp
    |-- studio_app.cpp
    |-- studio_host.hpp
    |-- studio_host.cpp
    |-- bridge/
    |   |-- engine_bridge.hpp
    |   `-- engine_bridge.cpp
    |-- commands/
    |   |-- command_router.hpp
    |   `-- command_router.cpp
    |-- transport/
    |   |-- http_server.hpp
    |   |-- http_server.cpp
    |   |-- websocket_server.hpp
    |   `-- websocket_server.cpp
```

## Ownership

### `StudioApp`

- owns `StudioHost`
- app lifecycle only

### `StudioHost`

- owns `EngineBridge`
- keeps future-facing transport pieces secondary
- creates CoreTools through the engine-owned web dialog API
- keeps future-facing transport pieces secondary
- runs the studio loop while the engine itself owns dialog layout updates

### `EngineBridge`

- remains the native engine-facing authority
- initializes, ticks, and shuts down `EngineCore`
- exposes the local CoreTools content path and access to the underlying `EngineCore`

### `CommandRouter`

- stays in the structure as the future command ingress
- is intentionally light in this milestone

### `transport/`

- preserves the future HTTP/WebSocket path
- is not active in this milestone

## Runtime Behavior

Current startup path:

1. `main()` creates `StudioApp`
2. `StudioApp::Initialize()` initializes `StudioHost`
3. `StudioHost::Initialize()` initializes `EngineBridge`
4. `EngineBridge` initializes `EngineCore`
5. `StudioHost` creates a docked-left web dialog through `EngineCore`
6. the engine `WebViewModule` creates and owns the WebView2 host surface
7. the WebView2 surface loads `apps/studio/coretools/runtime/index.html`
8. `StudioHost::Run()` keeps ticking the native engine while the engine updates docked dialog layout

Temporary stop paths:

- close the native window
- press `Escape`

## CoreTools

`CoreTools` is the first-party tool surface for the studio.
For this milestone it is intentionally minimal and only proves that embedded web-authored UI is rendering correctly inside the native host.

Visible content includes:

- a CoreTools title
- `Hello from CoreTools`

## CoreTools Frontend Source

Angular is intended to belong to the `CoreTools` module itself, not to the studio host root.

The current split is:

- `apps/studio/coretools/angular`
  - framework source and frontend tooling for `CoreTools`
- `apps/studio/coretools/runtime`
  - runtime web content loaded by WebView2
- `apps/studio/coretools/README.md`
  - module-level notes for how source and runtime content relate

That keeps the native build separate while also keeping frontend implementation details owned by the `CoreTools` module boundary.

## Engine Web Dialog API

This studio milestone now depends on the engine's reusable web dialog feature.

Current engine-level capabilities include:

- dock-left, dock-right, dock-top, dock-bottom, fill, and manual child placement inside the engine window
- floating top-level web dialogs
- local file-backed WebView2 content loading
- runtime visibility, bounds, and content-path updates through `EngineCore`

## Build Notes

The studio target is:

- `engine_studio`

Useful scripts:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja
cmake --build build --target engine_studio
```

The build copies `WebView2Loader.dll` from a known local Visual Studio path when that loader is available there.
If it is not copied automatically, place `WebView2Loader.dll` beside `engine_studio.exe`.

## Next Steps

- verify the engine-owned web dialog surface renders on the target machine
- add additional engine-level docking behaviors beyond the fixed presets
- decide how native main-area rendering should evolve alongside embedded tool surfaces
- reintroduce transport and command/event messaging as future editor-facing systems instead of browser-first runtime requirements
