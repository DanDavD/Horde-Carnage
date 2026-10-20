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
3. Run `Update` that many times, always with the same `dt`.
4. Draw once, passing `FixedTimestep::Alpha` (how far we are into the next step) so drawing can interpolate between two steps.

## Design decisions

- **A fixed timestep instead of updating with the raw frame time.** With a variable `dt`, a fast enemy can jump through a thin wall on a slow frame, and a dash covers a different distance at 60 and 144 FPS. With a fixed step, the same inputs always give the same result, which also makes gameplay bugs reproducible.
  - Rejected: a plain variable `dt`, because of the problems above.
  - Rejected: locking the game to 60 FPS with `SetTargetFPS`, because a slow frame would then slow the whole game down, and high refresh rate monitors would not get smoother rendering.
- **Clamping long frames to 0.25 s.** After a breakpoint or a window drag, the real frame time can be seconds. Without a clamp the loop would try to run hundreds of steps at once and fall further behind (the "spiral of death"). With it, the game just slows down for a moment.
- **raylib instead of an engine or a hand-written renderer.** I want my time to go into the game architecture, not into OpenGL setup and drivers. raylib is a small C library, so it does not decide how the game is structured.
  - Rejected: SDL2. It is just as valid, but needs more setup for 2D drawing and text.
- **Dependencies pinned with CMake's FetchContent.** A fresh clone builds with one command, with no package manager to install, and everyone gets the same versions.
- **Warnings as errors only for my code.** `horde_set_warnings` adds `/W4 /WX` to the project's targets. raylib and doctest are added as `SYSTEM`, so a warning in their headers cannot break my build.

## How to tune it

- The simulation rate and the frame clamp are the two constructor arguments of `FixedTimestep` (`1.0 / 60.0` and `0.25` by default). `Game` uses the defaults.
- The window size is `kScreenWidth` and `kScreenHeight` at the top of `src/app/Game.cpp`.

## Tests

- `tests/core/FixedTimestepTests.cpp`: exact steps, accumulation of short frames, the remainder of long frames (`Alpha`), the clamp on huge frames, negative frame times and the 60 Hz default.

## Limitations

- Rendering does not interpolate anything yet, because nothing moves except the title. `Alpha` is already passed to `Draw` for when entities exist.
- I only build and test on Windows with MSVC. The CMake setup should work with GCC and Clang, but I have not tried it.
