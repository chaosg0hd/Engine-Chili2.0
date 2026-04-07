# Angular Source

This folder now contains the Angular-owned source project for the embedded `CoreTools` module.

Current flow:

1. install dependencies inside `apps/studio/coretools/angular`
2. iterate with `npm run start`
3. open `http://localhost:4200/` in a browser during dev
4. build production output with `npm run build:runtime`
5. let `CoreToolsHost` continue loading `apps/studio/coretools/runtime/index.html`

Key files:

- `package.json`
  - local frontend scripts and Angular dependencies
- `angular.json`
  - Angular build configuration
- `src/`
  - CoreTools application source
- `scripts/sync-runtime.mjs`
  - copies Angular build output into `../runtime`

This project remains intentionally separate from CMake so the native studio build can stay independent from the frontend toolchain.
