# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - feature-test sandbox app
- `engine_studio` - browser-first studio host scaffold

Sanitizer build:

- `configure.cmd sanitize`
- `build.cmd sanitize`
- Or use the CMake presets `sanitize`, `sanitize-all`, `sanitize-sandbox`, and `sanitize-studio`

Studio status:

- `engine_studio` owns a native C++ backend host and a web UI under `apps/studio/web`
- The backend currently configures localhost endpoints for `http://127.0.0.1:3000` and `ws://127.0.0.1:8765`
- HTTP and WebSocket transport are still scaffolds, so the URLs are architectural targets, not live services yet
- The native studio window now stays responsive by ticking the engine frame loop while the web connectivity layer is being built out

See [docs/README.md](docs/README.md) for the current architecture, feature list, and API inventory.
