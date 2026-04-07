# Studio Documentation

## Overview

`engine_studio` is now a native-window-hosted editor shell for Engine-Chili2.0.
The studio keeps the existing native engine window and embeds a WebView2 surface
inside it as a fixed left sidebar for first-party CoreTools content.

Current architecture:

```text
engine_studio.exe
|-- native studio window
|-- native engine/main area
`-- left-docked WebView2 CoreTools surface
```

This milestone is Windows-only and intentionally focused:

- the native outer window stays authoritative
- `CoreTools` is the first-party embedded tool surface
- the left dock is fixed-width for now
- transport and command scaffolding remain in the repo, but they are not the runtime center of this milestone

## Current Structure

```text
apps/studio
|-- CMakeLists.txt
|-- README.md
|-- config/
|-- coretools/
|   |-- index.html
|   |-- app.js
|   |-- style.css
|   `-- panels/
|       `-- hello-panel.js
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
    |-- layout/
    |   |-- dock_layout.hpp
    |   `-- dock_layout.cpp
    |-- transport/
    |   |-- http_server.hpp
    |   |-- http_server.cpp
    |   |-- websocket_server.hpp
    |   `-- websocket_server.cpp
    `-- webview/
        |-- coretools_host.hpp
        `-- coretools_host.cpp
```

## Ownership

### `StudioApp`

- owns `StudioHost`
- app lifecycle only

### `StudioHost`

- owns `EngineBridge`
- owns `DockLayout`
- owns `CoreToolsHost`
- keeps future-facing transport pieces secondary
- runs the studio loop and updates the embedded surface layout as the native window changes size

### `DockLayout`

- computes a fixed left dock rectangle
- computes the remaining native main area rectangle
- does not implement drag docking, tabs, or floating windows yet

### `CoreToolsHost`

- owns the embedded WebView2 instance
- attaches it to the native studio window as a child region
- resizes it to match the left dock bounds
- loads local CoreTools content from `apps/studio/coretools/index.html`

### `EngineBridge`

- remains the native engine-facing authority
- initializes, ticks, and shuts down `EngineCore`
- exposes the native host window handle and local CoreTools content path

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
5. `StudioHost` computes the left dock bounds with `DockLayout`
6. `StudioHost` creates `CoreToolsHost`
7. `CoreToolsHost` creates an embedded WebView2 child surface
8. the WebView2 surface loads `apps/studio/coretools/index.html`
9. `StudioHost::Run()` keeps ticking the native engine while resizing the left dock as needed

Temporary stop paths:

- close the native window
- press `Escape`

## CoreTools

`CoreTools` is the first-party tool surface for the studio.
For this milestone it is intentionally minimal and only proves that embedded web-authored UI is rendering correctly inside the native host.

Visible content includes:

- a CoreTools title
- `Hello from CoreTools`

## Build Notes

The studio target is:

- `engine_studio`

Useful scripts:

```powershell
.\configure.cmd
.\build.cmd
```

The build copies `WebView2Loader.dll` from a known local Visual Studio path when that loader is available there.
If it is not copied automatically, place `WebView2Loader.dll` beside `engine_studio.exe`.

## Next Steps

- verify the embedded WebView2 surface renders on the target machine
- add additional native layout regions once the fixed left dock is stable
- decide how native main-area rendering should evolve alongside embedded tool surfaces
- reintroduce transport and command/event messaging as future editor-facing systems instead of browser-first runtime requirements
