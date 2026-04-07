# Studio Documentation

## Overview

`engine_studio` is the editor-facing host for Engine-Chili2.0.
It keeps engine authority in native C++ while the UI grows as a web frontend.

Current architecture:

```text
Web Frontend
    |
    | localhost HTTP / WebSocket
    v
Studio Backend Host (C++)
    |
    v
Engine Core (C++)
```

Current transport status:

- the host frontend lives under `apps/studio/webhost`
- the first-party tool bundle lives under `apps/studio/tools/coretools`
- the backend serves the studio web root over localhost
- the backend accepts real browser WebSocket connections over localhost

## Current Structure

```text
apps/studio
|-- CMakeLists.txt
|-- README.md
|-- config/
|-- runtime_test/
|-- src/
|   |-- main.cpp
|   |-- studio_app.hpp
|   |-- studio_app.cpp
|   |-- studio_host.hpp
|   |-- studio_host.cpp
|   |-- bridge/
|   |   |-- engine_bridge.hpp
|   |   `-- engine_bridge.cpp
|   |-- commands/
|   |   |-- command_router.hpp
|   |   `-- command_router.cpp
|   |-- events/
|   |   |-- event_bus.hpp
|   |   `-- event_bus.cpp
|   `-- network/
|       |-- http_server.hpp
|       |-- http_server.cpp
|       |-- websocket_server.hpp
|       `-- websocket_server.cpp
|-- webhost/
|   |-- index.html
|   |-- app.js
|   |-- style.css
|   `-- shell/
|       `-- runtime.js
`-- tools/
    `-- coretools/
        |-- manifest.json
        `-- index.js
```

## Backend Boundary

### `StudioApp`

- owns `StudioHost`
- handles app-level initialize, run, and shutdown

### `StudioHost`

- owns the studio runtime composition
- initializes `EngineBridge`
- configures the HTTP and WebSocket localhost services
- binds the command router
- binds the event bus to transport broadcast
- runs the backend host loop
- ticks the engine every frame so the native window stays responsive

### `EngineBridge`

- wraps `EngineCore` for studio use
- initializes, ticks, and shuts down the native engine
- owns backend shutdown authority
- provides studio-facing helper methods like:
  - studio web root lookup
  - hello message creation
  - exit requests

### `CommandRouter`

- receives backend command messages
- acts as the only command ingress
- routes known commands such as:
  - `hello`
  - `ping`
  - `get_status`
  - `exit`
- keeps backend authority over command validation and shutdown

### `EventBus`

- receives backend-originated notifications
- separates outbound studio events from inbound command handling
- lets `StudioHost` and `EngineBridge` publish notifications without depending on transport details

### `HttpServer`

- owns the configurable studio web root
- maps `/` to `webhost/index.html`
- serves HTML, JS, CSS, and other web assets over localhost
- stays transport-only and does not interpret studio commands

### `WebSocketServer`

- binds the localhost WebSocket endpoint
- accepts real browser clients
- receives text messages from clients
- forwards command payloads to `CommandRouter`
- sends responses back to the requesting client
- broadcasts backend events to connected clients
- stays transport-only and does not interpret command meaning

## Frontend Boundary

The browser frontend is now split into:

- `webhost/`
  - shell layout
  - browser runtime
  - shared socket connection
  - fixed dock-zone layout
  - panel registration
  - tool mounting
- `tools/coretools/`
  - first-party bootstrap tool pack
  - dock-aware panel descriptors
  - Hello, Ping, Status, and Exit actions
  - the same command path future tool bundles should use

Current target endpoints:

- `http://127.0.0.1:3000`
- `ws://127.0.0.1:8765`

Important limitation:

- these are live localhost transport endpoints
- the editor feature surface is still intentionally minimal while the platform boundary is being locked in

## Runtime Behavior

Current studio startup path:

1. `main()` creates `StudioApp`
2. `StudioApp::Initialize()` initializes `StudioHost`
3. `StudioHost::Initialize()` initializes `EngineBridge`
4. `EngineBridge` initializes `EngineCore`
5. `StudioHost` configures HTTP and WebSocket localhost services
6. `StudioHost` binds `CommandRouter` and `EventBus`
7. `StudioHost::Run()` opens the browser to the webhost when possible
8. `EngineBridge::Tick()` keeps the native engine window responsive each frame
9. `HttpServer::Tick()` serves browser requests
10. `WebSocketServer::Tick()` handles browser socket traffic
11. `StudioHost` stops when backend shutdown is requested

Temporary stop paths that currently work even before real networking:

- close the native window
- press `Escape`

## Current Frontend Flow

The minimal web shell currently aims for this flow:

1. Load the studio page
2. `webhost` bootstraps the browser runtime
3. `webhost` connects to `ws://127.0.0.1:8765`
4. `webhost` mounts `tools/coretools`
5. `coretools` sends `hello`, `ping`, `get_status`, or `exit`
6. Backend responses return through the shared command contract
7. Backend events return through the separate event lane
8. `exit` shuts the backend down

That flow is now intended to work over the real localhost transport path.

## Command Contract

The backend now expects command envelopes with this shape:

```json
{
  "kind": "command",
  "protocol_version": "1",
  "command": "get_status",
  "request_id": "studio-web-1",
  "sender": "studio-web-ui"
}
```

Current backend commands:

- `hello`
- `ping`
- `get_status`
- `exit`

Response envelopes use the same protocol version and include:

- `kind`
- `ok`
- `command`
- `request_id`
- `code`
- `message`

This keeps `CommandRouter` as the only command ingress and prevents transport layers from becoming ad-hoc feature hosts.

## Tool Pack Model

`CoreTools` is now treated as the first-party tool pack, not as a privileged internal page.

That means:

- `webhost` is the shell/runtime host page
- `tools/coretools` is mounted into that shell like any future tool bundle
- future tools should follow the same structure and backend contract
- tool bundles still talk through backend APIs rather than bypassing the host

## Dock Layout Infrastructure

The first dock implementation is intentionally simple and fixed.

Current dock zones:

- `top`
- `left`
- `center`
- `right`
- `bottom`

The host shell owns these zones as layout infrastructure.
Tool packs register panels into them instead of injecting arbitrary page content.

Current panel shape:

- `id`
- `title`
- `entry`
- `defaultDock`

`CoreTools` now mounts its bootstrap UI through docked panel registration rather than a one-off page mount.
That means the first Hello-world tool already follows the same editor pattern expected for future hierarchy, inspector, console, and toolbar panels.

## Event Flow

The backend now has a separate outbound event path:

```text
WebSocketServer
    -> CommandRouter
        -> EngineBridge / StudioHost services

StudioHost / EngineBridge
    -> EventBus
        -> WebSocketServer broadcast
```

Current event examples:

- `studio.started`
- `studio.hello`
- `shutdown.requested`
- `studio.stopping`

This keeps backend requests and backend notifications on separate lanes as the editor surface grows.

## Build Notes

The studio is built as part of the repo target set:

- `engine_studio`

Useful scripts:

```powershell
.\configure.cmd
.\build.cmd
```

Sanitizer path:

```powershell
.\configure.cmd sanitize
.\build.cmd sanitize
```

## Next Steps

- decide whether the studio should keep the native engine window visible or move toward a headless backend mode
- expand the command router beyond the current bootstrap commands
- add console/event streaming from backend to frontend
- add real editor panels such as hierarchy, inspector, and console
