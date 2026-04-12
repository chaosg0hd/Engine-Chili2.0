# CoreTools Module

`CoreTools` is the first-party embedded tool module for `engine_studio`.

Current structure:

- `runtime/`
  - active minimal placeholder runtime loaded by `engine_studio`
- `../archive/coretools-angular-spike-2026-04-07/`
  - archived Angular experiment, including source, generated runtime, logs, and `node_modules`

Current intent:

1. keep `coretools/runtime/` as the stable packaged surface for Studio
2. preserve the older Angular experiment under `apps/studio/archive/coretools-angular-spike-2026-04-07/`
3. keep isolated UI prototypes outside the live Studio module when we want clean separation
4. revisit a server-backed Angular runtime path later if needed

The live Studio module stays intentionally small so the native build and packaged runtime remain easy to reason about.
