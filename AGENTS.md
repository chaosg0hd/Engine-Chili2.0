# Codex Instructions

## Build And Configure Commands

Do not run CMake, Ninja, configure, build, or test commands in the normal CLI sandbox for this repository.

Always request escalated execution before running commands such as:

- `cmake -S . -B build -G Ninja`
- `cmake --build build`
- `cmake --build build --target ...`
- `configure.cmd`
- `build.cmd`

Reason: this Windows/DX-oriented repo can stall or behave incorrectly when CMake/Ninja build subprocesses run inside the CLI sandbox.

Inspection commands such as `rg`, `Get-Content`, `git status`, and file listing commands are fine in the sandbox. Build/configure/test commands are not.

## Build Architecture Direction

The project is moving away from hot-building monolithic executable targets as the normal workflow.

Future build/runtime work should aim for:

```txt
launcher.exe
    -> engine.dll
    -> app/editor/game DLLs
```

Ownership:

```txt
launcher exe owns process startup, DLL loading, and handoff only.
engine.dll owns reusable engine systems, modules, and runtime coordination.
app/editor/game DLLs own project-specific behavior loaded after the engine.
```

Do not hide new runtime/editor behavior in the launcher just because it is convenient.
Prefer DLL-safe boundaries, explicit exported entry points, and reload-friendly ownership when changing build architecture.
App-specific runtime DLLs and preview hosts should live with the app folder they belong to, such as `apps/pong`, rather than in generic folders that hide ownership.
Build outputs should preserve that ownership too: engine outputs under `build/bin/engine`, Studio under `build/bin/studio`, app outputs under `build/bin/apps/<app_name>`, and external tools under `build/bin/tools/<tool_name>`.

## User Workspace Proxy

`User/` is a repo-local proxy for the future user documents/workspace folder.

Treat it as generated/user-owned project data, not engine source truth.

Rules:

```txt
User/ may contain created/opened Studio projects and other user workspace artifacts.
User/ should not be used as the durable location for engine templates, app source, or runtime systems.
Clearing User/ means clearing generated workspace contents, not deleting engine-owned features.
Keep the User/ directory itself available as the local workspace mount point.
```

When adding project-management behavior, write through the Studio project/file systems and keep the eventual real documents-folder destination in mind.

## Current Work Direction

The engine architecture is being fleshed out through real game trials. Treat `apps/pong`, future game trials, and `apps/_template` as front-facing pressure tests for what app authors and players actually see and do.

When game work exposes missing capabilities, awkward APIs, or presentation gaps, prefer improving the architecture in small reusable steps instead of hiding those issues inside one app.

## Codex Operating Checklist

Before changing code, trace the real path:

```txt
runtime truth -> prototype intent -> render lowering -> GPU/backend execution -> screen
```

For every feature, name the owner before editing:

```txt
App/GameRuntime owns gameplay state and rules.
Prototypes own reusable visual intent.
RenderModule owns frame translation.
GpuModule owns backend execution.
Studio owns developer control and visibility only.
```

When an app trial exposes missing engine language or awkward usage, fix the smallest reusable engine/API gap instead of hiding the problem in the app.

Do not accept app code that has to know renderer internals, raw packed colors, GPU concepts, resource decoding, shader bindings, or backend details.

Do not add Studio fake previews or duplicate runtime logic. If Studio displays it, it must come from the real project/runtime path.


# Engine-Chili2.0 — Agent Instructions

## Prime Directive

This engine is designed for **cheap future extension through correct architecture**.

Agents must optimize for:

```txt
ownership correctness
clear data flow
reusable language (prototypes)
clean app-level expression
```

Agents must NOT optimize for:

```txt
shortest path
local feature success
temporary hacks becoming permanent
```

A feature that works but violates ownership or flow is considered **incorrect**.

---

# Core Philosophy

## 1. Architecture exists to reduce future cost

Every feature must:

```txt
work today
make similar features cheaper tomorrow
not create hidden coupling
not break unrelated systems later
```

If a feature introduces duplication, hardcoding, or bypasses ownership, it increases future cost and is invalid.

---

## 2. Think in flows, not files

Before coding, define the full pipeline:

```txt
input → runtime → prototype → renderer → GPU → screen
```

or:

```txt
project → runtime → frame → render → viewport
```

If you cannot describe the flow, do not implement.

---

## 3. Ownership is absolute

Each system has a clear owner:

```txt
GameRuntime → gameplay state & logic
Prototypes → reusable intent language
RenderModule → frame translation
GpuModule → execution
Studio → control + visibility
ProjectPackage → project identity
```

Never move logic to a non-owning layer for convenience.

---

## 4. Prototypes are the engine’s language

Prototypes are:

```txt
data-only
composable
reusable
backend-agnostic
```

They are NOT:

```txt
gameplay state
behavior containers
engine internals
GPU objects
```

Rule:

```txt
If a concept describes reusable visual or presentation intent,
it belongs in prototypes.
```

---

## 5. App code expresses intent

App/game/runtime code should look like:

```cpp
on_update(dt)
{
    ball.pos += ball.vel * dt;

    if (outside(ball, arena))
        bounce(ball);
}
```

NOT:

```cpp
RenderItemPrototype item;
item.transform.position.x = ...
renderer.Submit(...)
```

If app code is verbose, the abstraction is wrong.

---

# Required Work Process

## Step 1 — Define the feature flow

Example:

```txt
Ball state → BallPrototype → FramePrototype → RenderModule → GPU
```

## Step 2 — Identify ownership

Answer:

```txt
Who owns state?
Who owns execution?
Who owns translation?
```

## Step 3 — Trace real code path

Follow:

```txt
data origin → transformation → consumption
```

Do not assume.

## Step 4 — Fix the real pipeline

Do NOT:

```txt
create fake previews
hardcode demos
duplicate systems
```

Fix the missing bridge.

## Step 5 — Expand prototypes if needed

If the engine lacks expressive language:

```txt
add minimal reusable prototype
compose it into existing system
```

---

# Prototype Expansion Rule

Allowed:

```txt
ShapePrototype
Circle / Sphere primitives
BallPrototype (if purely descriptive)
```

Forbidden:

```txt
Ball with velocity, scoring, logic
anything with behavior or simulation
```

---

# App vs Engine Boundary

## App owns:

```txt
game rules
simulation logic
state updates
event handling
```

## Engine owns:

```txt
rendering
GPU
resource loading
memory
platform
threading
```

---

# Studio Role

Studio is for:

```txt
developers
```

It must:

```txt
load real projects
run real runtime
display real frames
control execution (play/pause)
```

It must NOT:

```txt
simulate gameplay
render fake demos
own game state
bypass runtime
```

Rule:

```txt
If Studio shows something, it must come from the real runtime.
```

---

# Project Package Rule

A game is a package.

It defines:

```txt
identity
paths
runtime entry
startup scene
```

Pipeline:

```txt
Open Project
    → ProjectPackage
    → RuntimeLaunchInfo
    → RuntimeHost
    → GameRuntime
    → FramePrototype
    → Renderer
```

---

# Rendering Rule

Rendering is always:

```txt
FramePrototype
    → RenderModule
    → RenderFrameData
    → GpuModule
    → backend execution
```

Never bypass this.

---

# App-Level Coding Style (CRITICAL)

App code must be:

```txt
concise
expressive
low-boilerplate
operator-friendly
```

## Allowed patterns

### Direct data access

```cpp
ball.pos += ball.vel * dt;
```

### Operator overloading

```cpp
pos += vel * dt;
```

### Free functions

```cpp
bounce(ball);
```

### Event hooks

```cpp
on_update(dt) { ... }
on_collision(a, b) { ... }
```

### Simple structs

```cpp
struct Ball
{
    vec2 pos;
    vec2 vel;
    float radius;
};
```

### Loops and logic

```cpp
for (auto& enemy : enemies)
    enemy.pos.y += enemy.speed * dt;
```

---

## Strongly preferred features

```txt
defer-style cleanup
value semantics
minimal ceremony
```

---

## Forbidden patterns

```txt
getters/setters everywhere
deep class hierarchies
Java-style verbosity
manual engine plumbing
render calls in app code
GPU/backend exposure
```

---

# App API Rule

If app code becomes verbose:

```txt
DO NOT accept it
```

Instead:

```txt
create/refine app-facing API
```

Example:

```cpp
app.present.ball(pos, radius, "white");
```

instead of:

```cpp
RenderItemPrototype + manual wiring
```

---

# Low-Level Boundary Rule

App, runtime, and studio must NEVER see:

```txt
0xFFAA2233 color packing
vertex buffers
GPU calls
DX11/Vulkan
shader bindings
resource decoding
```

Rule:

```txt
If the developer has to think about how it's drawn, the engine has failed.
```

---

# Anti-Patterns

Reject any implementation that:

```txt
hardcodes runtime
duplicates rendering paths
adds fake preview systems
bypasses module ownership
pushes logic into EngineCore
stores features only in app layer
leaks GPU/backend details upward
```

---

# Final Principles

```txt
Game defines WHAT happens
Prototypes define WHAT it looks like
Engine defines HOW it runs
Studio lets you see/control it
```

```txt
If it feels like writing enterprise Java, it is wrong.
```

```txt
If a feature is not reusable, question its placement.
```

```txt
If you bypass the pipeline, you are breaking the engine.
```

```txt
Always build the real path. Never fake it.
```

---

# Hypothetical Feature Flowchart

Use this flow every time a new feature is requested.

Example request:

```txt
"Make me a ball in the game."
```

## Feature Decision Flow

```txt
START
  |
  v
What is the user asking for?
  |
  v
Is it gameplay behavior, visual intent, tool behavior, or engine infrastructure?
  |
  +--> Gameplay behavior
  |       |
  |       v
  |   App/GameRuntime owns rules and state
  |       |
  |       v
  |   Does it need visible output?
  |       |
  |       +--> Yes -> express output through prototypes
  |       +--> No  -> keep it runtime-only
  |
  +--> Visual / presentation intent
  |       |
  |       v
  |   Can existing prototypes express it?
  |       |
  |       +--> Yes -> compose existing prototypes
  |       |
  |       +--> No
  |             |
  |             v
  |        Add smallest reusable passive prototype
  |             |
  |             v
  |        Lower it into existing FramePrototype path
  |
  +--> Tool / Studio behavior
  |       |
  |       v
  |   Studio may control or inspect
  |       |
  |       v
  |   Studio must not own game/runtime truth
  |
  +--> Engine infrastructure
          |
          v
      Find owning module
          |
          v
      Add/refine narrow capability boundary
```

## Ball Example

```txt
Request:
"Make me a ball."

Step 1:
Ball has gameplay meaning.
Runtime owns:
- position
- velocity
- collision
- rules

Step 2:
Ball also needs visible output.
Prototype layer owns:
- ball visual description
- shape
- radius
- material/style
- transform

Step 3:
Renderer owns:
- lowering prototype to render data
- GPU submission

Step 4:
App code only fills arguments.
```

Correct flow:

```txt
PongRuntime
    owns BallState
        |
        v
BallState updates with dt
        |
        v
PongRuntime creates BallPrototype
        |
        v
BallPrototype composes Shape/Material/Transform prototypes
        |
        v
BallPrototype lowers into FramePrototype
        |
        v
RenderModule lowers FramePrototype into RenderFrameData
        |
        v
GpuModule/backend executes draw
        |
        v
Studio viewport or game window displays ball
```

## Code Shape

Good app-level code:

```cpp
on_update(dt)
{
    ball.pos += ball.vel * dt;

    if (outside(ball, arena))
        bounce(ball);

    present(ball);
}
```

Good prototype-facing code:

```cpp
BallPrototype view;
view.pos = ball.pos;
view.radius = ball.radius;
view.style = "default_ball";

frame.add(view);
```

Bad code:

```cpp
renderer.DrawCircle(ball.pos.x, ball.pos.y, ball.radius, 0xFFFFFFFF);
```

Reason:

```txt
The app leaked into render implementation.
The renderer API leaked into gameplay.
The prototype language was bypassed.
```

## Final Feature Rule

```txt
When adding a feature, always ask:

1. What is the gameplay/runtime truth?
2. What is the reusable prototype expression?
3. What module lowers it?
4. What module executes it?
5. What high-level app API should make it pleasant to use?
```

If the answer requires app code to touch renderer internals, GPU concepts, raw colors, buffers, or backend details, the API is too low-level.
