# Build Lanes (Current Direction)

This document describes the current build reality while the repository is still converging on a unified builder contract.

## Why This Exists

The repo currently serves multiple users and automation surfaces that do not all share the same ideal entry point yet.

Treat this as an architecture transition period, not a final policy lock.

## Supported Build Lanes

### 1) GitHub / CI lane

Goals:

- deterministic outputs
- automation-safe behavior
- no interactive assumptions

Current shape:

- Windows + MSVC + Ninja
- non-interactive execution
- reproducible logs/artifacts

### 2) Codex lane

Goals:

- agent-friendly terminal flow
- clear logs
- tolerant of repeated runs and repo scanning

Current shape:

- wrapper-script and raw-CMake paths can both appear during transition
- commands must run outside restricted CLI sandbox for this repo

### 3) Claude lane

Goals:

- agent-friendly explicit instructions
- minimal hidden assumptions
- easy recovery when context is partial or stale

Current shape:

- same backend build graph as other lanes
- documentation must remain explicit about required environment and command expectations

### 4) Human idiot-proof lane

Goals:

- simplest possible entry point
- one obvious command or builder script where possible
- clear errors without requiring CMake/Ninja expertise

Current shape:

- wrapper scripts are the easiest user-facing path today
- lower-level commands may still be used by advanced users/tools

## Convergence Goal (Target Architecture)

The long-term target is one builder contract that all lanes delegate to:

- one underlying build definition
- one canonical policy surface for configure/build/export behavior
- one primary command/script entry where practical
- wrappers, Studio, CI, and agents acting as clients of that shared contract

Important transition rule:

- do not claim convergence is complete until wrappers, Studio build flows, and CI all resolve through the same contract implementation.

## Studio Build Policy (Transition Note)

`StudioBuildSystem` is currently a build client with a temporary direct CMake backend.

That is acceptable during transition, but:

- raw command assembly must stay centralized
- build policy should not be scattered across unrelated Studio code paths
- Studio must eventually delegate to the unified builder contract instead of owning build policy
