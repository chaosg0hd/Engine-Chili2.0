# CoreTools Module

`CoreTools` is the first-party embedded tool module for `engine_studio`.

Current structure:

- `runtime/`
  - active minimal placeholder runtime loaded by `engine_studio`
- `../archive/coretools-angular-spike-2026-04-07/`
  - archived Angular experiment, including source, generated runtime, logs, and `node_modules`

Current intent:

1. keep `coretools/runtime/` as the stable packaged surface for Studio
2. preserve the Angular experiment under `apps/studio/archive/coretools-angular-spike-2026-04-07/`
3. revisit a server-backed Angular runtime path later if needed

The Angular spike has been intentionally archived for now because the direct `file://` runtime path is not the right fit for that build output yet.
