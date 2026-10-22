# Architecture

## What it is

How the code of Horde Carnage is split, and why. This document only describes what exists in the code today. Planned pieces are listed in the [roadmap](ROADMAP.md).

## How it works

### Two targets

The build makes two things:

- **`horde_core`**, a static library with the pure game logic. It never opens a window, reads input or draws, so the unit tests can link it and run anywhere, including CI.
- **`HordeCarnage`**, the game executable. It owns the window (raylib), turns input into commands for the logic, and draws the result.

```
tests/ (horde_tests)  --->  horde_core  <---  HordeCarnage  --->  raylib
```

When a feature has rules (damage, cooldowns, wave timing), the rules go into `horde_core` with tests. The executable only connects them to the screen and the keyboard.

### The main loop

`Game::Run` (`src/app/Game.cpp`) does this every frame:

1. Ask raylib how long the last frame took (`GetFrameTime`).
2. Give that time to `FixedTimestep::Advance`, which answers how many fixed 1/60 s steps are due.
3. Run `Update` that many times, always with the same `dt`. `Update` reads the input (`ReadMoveInput`) and passes it to `StepWorld`, which runs all the rules.
4. Draw once, passing `FixedTimestep::Alpha` (how far we are into the next step) so drawing can interpolate between two steps.

### The world

`World` (`src/core/World.h`) holds everything the simulation needs as plain data: the arena, the player, the enemies and the camera. Two functions work on it:

- `CreateWorld` builds the starting state: the player in the middle of the arena, the hairballs around it, the camera on the player.
- `StepWorld` advances it by one fixed step, in this order: the player (facing, movement), then every enemy (chase, movement), then collisions between bodies and the walls (four passes, see [Collisions](COLLISIONS.md)), then the camera.

`Game` owns one `World`. It reads the input, calls `StepWorld`, and draws the result. It never changes the world itself.

### Components and systems

Game objects are plain structs (components) that hold data, and free functions (systems) that update them. The first ones are the `Motion` component and the `StepMovement`, `UpdateFacing` and `ConfineToArena` systems, described in [Movement](MOVEMENT.md). The camera follows the same shape: a `FollowCamera` component and a `StepFollowCamera` system, described in [Camera](CAMERA.md). The app only turns it into a raylib `Camera2D` when it draws. `Player` and `Enemy` are structs of components, not classes with behaviour, and they share the same movement systems (see [Enemies](ENEMIES.md)). `horde_core` has its own `Vec2` type, so it never includes raylib; the app converts with `ToRaylib` right before drawing.

## Design decisions

- **A fixed timestep instead of updating with the raw frame time.** With a variable `dt`, a fast enemy can jump through a thin wall on a slow frame, and a dash covers a different distance at 60 and 144 FPS. With a fixed step, the same inputs always give the same result, which also makes gameplay bugs reproducible.
  - Rejected: a plain variable `dt`, because of the problems above.
  - Rejected: locking the game to 60 FPS with `SetTargetFPS`, because a slow frame would then slow the whole game down, and high refresh rate monitors would not get smoother rendering.
- **Clamping long frames to 0.25 s.** After a breakpoint or a window drag, the real frame time can be seconds. Without a clamp the loop would try to run hundreds of steps at once and fall further behind (the "spiral of death"). With it, the game just slows down for a moment.
- **raylib instead of an engine or a hand-written renderer.** I want my time to go into the game architecture, not into OpenGL setup and drivers. raylib is a small C library, so it does not decide how the game is structured.
  - Rejected: SDL2. It is just as valid, but needs more setup for 2D drawing and text.
- **Dependencies pinned with CMake's FetchContent.** A fresh clone builds with one command, with no package manager to install, and everyone gets the same versions.
- **The whole simulation step lives in `horde_core` (`StepWorld`), and the app only reads input and draws.** A full game step can then run in a unit test, without a window. One test plays 400 steps twice with the same inputs and checks that both worlds end identical, which is the fixed timestep's promise put to the test. Restarting the game (later on day 3) is creating a fresh `World`, so nothing from the last run can leak into the next.
  - Rejected: keeping the update logic in `Game`. It would grow into a class that does everything, and could only be tested by playing.
- **Warnings as errors only for my code.** `horde_set_warnings` adds `/W4 /WX` to the project's targets. raylib and doctest are added as `SYSTEM`, so a warning in their headers cannot break my build.

## How to tune it

- The simulation rate and the frame clamp are the two constructor arguments of `FixedTimestep` (`1.0 / 60.0` and `0.25` by default). `Game` uses the defaults.
- The window size is `kScreenWidth` and `kScreenHeight` at the top of `src/app/Game.cpp`.

## Tests

- `tests/core/FixedTimestepTests.cpp`: exact steps, accumulation of short frames, the remainder of long frames (`Alpha`), the clamp on huge frames, negative frame times and the 60 Hz default.
- `tests/core/Vec2Tests.cpp`, `MovementTests.cpp`, `FacingTests.cpp`, `ArenaTests.cpp` and `AnalogInputTests.cpp`: see [Movement](MOVEMENT.md#tests).
- `tests/core/FollowCameraTests.cpp`: see [Camera](CAMERA.md#tests).
- `tests/core/ChaseTests.cpp` and `WorldTests.cpp`: see [Enemies](ENEMIES.md#tests).
- `tests/core/CollisionTests.cpp` and the crowd cases in `WorldTests.cpp`: see [Collisions](COLLISIONS.md#tests).

## Limitations

- I only build and test on Windows with MSVC. The CMake setup should work with GCC and Clang, but I have not tried it.
