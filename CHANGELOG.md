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
- **Movement model** (`Movement.h`, `Movement.cpp`): a `Motion` component and a `StepMovement` function that accelerate toward a target velocity and brake at a stronger rate when the input is released or points against the velocity, so turning back takes 0.175 s instead of 0.2 s. Input is clamped to length 1, so a keyboard diagonal is not faster than a straight line and a half-pushed stick gives half speed. Covered by `MovementTests`, explained in `docs/MOVEMENT.md`.
- **Player in the arena** (`Player.h`, `Game.cpp`): the placeholder title screen is replaced by an arena with a floor grid and walls, and a player circle you can move. The player is drawn between the last two fixed steps, so movement is smooth above 60 Hz. `MotionAt` spawns a mover with both positions set, so its first frame is not drawn from the origin. Covered by `MovementTests`.
- **Arena walls** (`Arena.h`, `Arena.cpp`): `ConfineToArena` keeps a circle inside the arena and cancels only the velocity that points into a wall, so the player slides along walls instead of sticking. Covered by `ArenaTests`.
- **Keyboard and gamepad input** (`PlayerInput.cpp`, `AnalogInput.h`): WASD, the arrow keys, and the left stick and d-pad of the first gamepad. The stick goes through a radial deadzone that keeps diagonals and rescales from zero, so there is no jump in speed. Covered by `AnalogInputTests`.
- **Facing** (`Facing.cpp`, `Player.h`, `Game.cpp`): the player faces the way the input points and keeps that direction when it stops, shown by an eye on the player's edge. It follows the input, not the velocity, so the melee swing on day 4 goes where the player is pushing now. Covered by `FacingTests`.
- **Movement debug overlay** (`Game.cpp`): in Debug builds only, `F1` shows the input, speed, velocity, facing and position, and draws the velocity and facing as lines, so I can tune movement against numbers. Not unit tested; checked in a Debug build.
