# Changelog

This file lists the notable changes to the project. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow [Semantic Versioning](https://semver.org/) while the game is a prototype (see [CONTRIBUTING](CONTRIBUTING.md#releases)).

## [Unreleased]

### Added
- **Build setup** (`CMakeLists.txt`, `CMakePresets.json`, `cmake/Dependencies.cmake`): a CMake project in C++20 that downloads raylib 5.5 and doctest 2.4.11 at configure time, pinned to those releases, so a fresh clone builds with one command. Presets for Debug and Release.
- **Strict warnings** (`cmake/Warnings.cmake`): `/W4 /WX /permissive-` on MSVC (and `-Wall -Wextra -Wpedantic -Werror` elsewhere) for the project's own targets only, so third-party code does not break the build.
- **Fixed-timestep loop** (`FixedTimestep.h`, `FixedTimestep.cpp`, `Game.cpp`): the game simulates at a fixed 60 Hz and renders once per frame, so gameplay does not depend on the monitor's refresh rate. Long frames are clamped to 0.25 s to avoid a spiral of catch-up steps. Covered by `FixedTimestepTests`.
- **Game window** (`Game.h`, `Game.cpp`, `main.cpp`): a 1280x720 window with vsync and a placeholder title screen whose bobbing title is driven by simulation time.
- **Tests and CI** (`tests/`, `.github/workflows/ci.yml`): doctest unit tests for the pure logic library `horde_core`, run by CTest, and a GitHub Actions workflow that builds Debug and Release and runs the tests on every push to `main` and every pull request.
- **Docs**: README, CONTRIBUTING, `docs/ARCHITECTURE.md`, `docs/ROADMAP.md` and a pull request template.
- **Vector math** (`Vec2.h`): a small 2D vector type for `horde_core`, so the pure logic and its tests do not depend on raylib. Covered by `Vec2Tests`.
- **Movement model** (`Movement.h`, `Movement.cpp`): a `Motion` component and a `StepMovement` function that accelerate toward a target velocity and decelerate when the input is released. Input is clamped to length 1, so a keyboard diagonal is not faster than a straight line and a half-pushed stick gives half speed. Not wired into the game yet. Covered by `MovementTests`, explained in `docs/MOVEMENT.md`.
