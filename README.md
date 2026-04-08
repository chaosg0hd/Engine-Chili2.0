# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - feature-test sandbox app
- `engine_studio` - native studio host with an embedded WebView2 CoreTools surface

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

See [docs/README.md](docs/README.md) for the current architecture, feature list, and API inventory.
