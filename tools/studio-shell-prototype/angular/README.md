# Studio Angular Prototype

This workspace is the isolated Angular Material prototype for the Studio shell.

Current intent:

1. keep frontend iteration under `apps/studio/coretools/angular`
2. stay outside `build/` and outside the native CMake pipeline
3. use `npm run build:runtime` only when we decide to push prototype output into `apps/studio/coretools/runtime`

Suggested flow:

```powershell
cd apps/studio/coretools/angular
npm install
npm run start
```

When you want to export the prototype into the local file-backed runtime:

```powershell
npm run build:runtime
```
