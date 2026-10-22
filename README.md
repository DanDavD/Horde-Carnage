# Horde Carnage

A top-down hack and slash about surviving a horde, written in C++20 with raylib.

Waves of monsters chase you around an arena. You cut through them, pick up what they drop, level up, change how your weapon works, and at the end a boss shows up.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)
![raylib](https://img.shields.io/badge/raylib-5.5-black)
![Status](https://img.shields.io/badge/status-in%20development-orange)

---

## The idea

I'm building Horde Carnage as a vertical slice in 15 days (October 20 to November 3, 2026), inspired by games like Castle Crashers, The Binding of Isaac and Dungeon Rampage.

I chose plain C++ with a small library instead of an engine on purpose. raylib only gives me a window, input, 2D drawing and audio. Everything that makes it a game (the loop, entities, collisions, enemy AI, waves, progression) is my own code, and that is the part I want to show.

## Current status

This is day 3 of 15. Progress is tracked in the [roadmap](docs/ROADMAP.md) and the [changelog](CHANGELOG.md).

**What works today:**
- A CMake project that downloads raylib and doctest at a pinned version, so a fresh clone builds with one command.
- A window with a fixed-timestep main loop: the game simulates at 60 Hz whatever the monitor's refresh rate, and renders once per frame.
- A player you move around an arena with the keyboard or a gamepad. It speeds up and stops over a fraction of a second, turns back quickly, is never faster on a diagonal, faces the way you push, and slides along the walls. See [Movement](docs/MOVEMENT.md).
- An arena bigger than the screen, with a camera that follows the player smoothly, looks a little ahead of where you run, and stops at the walls. See [Camera](docs/CAMERA.md).
- Six hairballs that come at you from every side and stop when they touch you. They are slower than you, so you can run away. See [Enemies](docs/ENEMIES.md).
- Bodies push each other apart instead of stacking. You can shove one hairball out of your way, but a crowd slows you down. See [Collisions](docs/COLLISIONS.md).
- Unit tests for the pure logic, and a GitHub Actions workflow that builds and tests every push and pull request on Windows.
- Warnings are errors for the project's own code (`/W4 /WX` on MSVC).

**Still to do:** health, contact damage and game over (the rest of day 3), then combat, waves and everything else that makes it a game. See the [roadmap](docs/ROADMAP.md).

## Tech stack

| Area | Choice |
|---|---|
| Language | C++20 |
| Window, input, 2D rendering, audio | [raylib](https://www.raylib.com/) 5.5 |
| Build | CMake 3.25+ with presets |
| Tests | [doctest](https://github.com/doctest/doctest) 2.4.11 |
| CI | GitHub Actions (Windows, MSVC) |

## Getting started

**You need:**
- Windows: Visual Studio 2026 (or 2022) with the **Desktop development with C++** workload. It includes MSVC, the Windows SDK and CMake.
- Other platforms: any C++20 compiler and CMake 3.25 or newer. I only test on Windows for now.
- An internet connection for the first configure, which downloads raylib and doctest.

From a *Developer PowerShell for VS*, in the repository folder:

```
cmake --preset default
cmake --build --preset release
build\default\src\Release\HordeCarnage.exe
```

You can also open the folder in Visual Studio (*File > Open > Folder*). It reads `CMakePresets.json` by itself.

**Controls:** `WASD` or the arrow keys move, and so do the left stick and the d-pad of a gamepad. `Esc` quits. In Debug builds, `F1` shows a movement overlay.

## Building and testing

- **Debug build:** `cmake --build --preset debug`. It keeps a console window with raylib's log.
- **Release build:** `cmake --build --preset release`. A plain windowed app, no console.
- **Tests:** `ctest --preset release` (or `--preset debug`). The tests only cover code in `horde_core`, which never opens a window.

## Project structure

```
CMakeLists.txt        Project root: C++ standard, options, dependencies
CMakePresets.json     Configure, build and test presets
cmake/
  Dependencies.cmake  raylib and doctest, pinned with FetchContent
  Warnings.cmake      Strict warnings for the project's own targets
src/
  core/               Pure game logic, no window (library horde_core)
  app/                The game: window, main loop, rendering (executable HordeCarnage)
  main.cpp
tests/
  core/               Unit tests for src/core
docs/                 Design and architecture notes
```

See [Architecture](docs/ARCHITECTURE.md) for how the pieces fit together.

## Roadmap

The full day-by-day plan, with what I cut first if I fall behind, is in [docs/ROADMAP.md](docs/ROADMAP.md).

- [x] `v0.0.1` Project setup: CMake, raylib, tests, CI, fixed-timestep loop
- [ ] `v0.1.0` Core loop: movement, enemies that chase you, melee combat, waves, game over
- [ ] `v0.2.0` Progression: XP, level-ups, weapons, data-driven enemies and weapons
- [ ] `v0.3.0` Vertical slice: power-ups, the boss, game feel, menus

## Contributing

This is a solo project, but I work on it like a small team would. See [CONTRIBUTING.md](CONTRIBUTING.md).
