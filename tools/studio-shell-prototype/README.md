# Studio Shell Prototype

This folder is the isolated workspace for Studio UI experiments that should stay outside the live `apps/studio` module.

Current layout:

- `angular/`
  - Angular Material prototype app
- `runtime/`
  - optional local build output produced by `npm run build:runtime`

Suggested flow:

```powershell
cd tools/studio-shell-prototype/angular
npm install
npm run start
```

This keeps Studio shell exploration independent from:

- `build/`
- `apps/studio/coretools/runtime`
- the native CMake pipeline
