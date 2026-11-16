# Horde Carnage — Custom C++20 Boss Fight Engine

A top-down boss fight game built in C++20 on top of raylib 5.5, combining arcade twin-stick shooting mechanics with Souls-like difficulty, telegraph-based boss AI, and deterministic simulation.

- **Play in Browser (WebAssembly):** [dandavd.itch.io/horde-carnage-demo](https://dandavd.itch.io/horde-carnage-demo)
- **Windows Release (v1.0.0):** [github.com/DanDavD/Horde-Carnage/releases/latest](https://github.com/DanDavD/Horde-Carnage/releases/latest)
- **Gameplay Demonstration:** [youtu.be/Oz9MaF9-lhU](https://youtu.be/Oz9MaF9-lhU)
- **Author:** Daniel David ([@DanDavD](https://github.com/DanDavD))
- **Stack:** C++20, raylib 5.5, CMake, Emscripten, GitHub Actions, doctest

---

## Executive Summary

- **Custom Engine Architecture:** raylib is utilized strictly as a thin platform abstraction layer for windowing, raw input, 2D rendering primitives, and audio output. The game loop, entity component models, AABB collision system, boss behavior state machines, and procedural audio synthesis are written from scratch.
- **Production QA & Testing:** Backed by **446 automated tests** (14,359 assertions) executing on a window-free headless core library. Built with strict compiler warnings-as-errors (`/W4 /WX`) and multi-platform CI building for Windows and WebAssembly.
- **Engineering Discipline:** Solo project planned and shipped with professional team practices: feature branching, peer pull requests, automated CI, and semantic versioning.

---

## Systems Engineering Highlights

### 1. Layered State Machine Boss AI
- The boss ("The Matriarch") operates across distinct overarching phases (Phase 1, Transformation, Phase 2, Frenzy).
- Attacks follow strict telegraph state lifecycles: **Warning -> Active -> Recovery**.
- Employs distance-weighted random attack selection with non-repeating combo constraints and alternating protective shields (cracked differently by pistol vs. SMG stream).

### 2. Deterministic 60 Hz Simulation
- Implements a fixed 60 Hz physics/logic simulation step with interpolated rendering.
- All pseudo-random mechanics derive from a seeded **PCG32 generator**, guaranteeing that identical inputs produce bit-exact deterministic combat runs for replayability and regression testing.
- Particle systems and audio playback pull from isolated RNG streams to ensure visual/auditory effects never desynchronize gameplay state.

### 3. Decoupled Headless Core (`horde_core`)
- All game rules, entity state, collision math, and combat logic live in `src/core/` (`horde_core`), completely decoupled from any window, GPU context, or raylib rendering headers.
- The presentation executable (`src/app/`) acts merely as a thin consumer translating user input into commands and drawing render frames.
- Enables the entire 446-test suite to execute headlessly in milliseconds during CI builds.

### 4. Zero-Allocation Object Pools
- Employs fixed-size contiguous memory object pools for bullets, enemy strands, shockwaves, summon markers, hairballs, events, and 512 particle instances.
- **Zero dynamic memory allocations (heap `malloc`/`new`) occur during active combat**, preventing GC stutter, heap fragmentation, and frame drops.

### 5. 100% Procedural Audio & Visual Pipeline
- **Zero External Media Files:** Built entirely without external raster sprites or audio sample files.
- Visual elements and animations are drawn programmatically using mathematical primitives and code-driven geometric transforms.
- Procedural audio synthesizer generates 25 distinct sound effects and a dynamic 5-layer adaptive metal score that modulates intensity based on boss phases and player health.

### 6. WebAssembly Compilation (Emscripten)
- Fully cross-compiled to WebAssembly using Emscripten.
- Overcame browser WebAssembly stack limits by refactoring memory layouts to prevent 64 KB stack overflows into engine globals.

---

## Architecture & Codebase Structure

```text
Horde-Carnage/
├── src/
│   ├── core/                  # Pure game logic and deterministic domain (horde_core library)
│   └── app/                   # Windowing, main loop, input polling, audio output (executable)
├── tests/
│   └── core/                  # 446 unit tests verifying horde_core logic via doctest
├── web/                       # WebAssembly shell HTML/JS for browser execution
├── docs/                      # Architectural design docs (Boss, Shield, Random, Object Pool, Audio)
└── CMakeLists.txt             # Multi-platform CMake build orchestrator
```

---

## Controls

| Action | Keyboard & Mouse | Gamepad |
| :--- | :--- | :--- |
| **Move** | `WASD` or Arrow keys | Left Analog Stick |
| **Aim & Fire** | Mouse cursor + Left Click (Hold for SMG) | Right Analog Stick + `RT` |
| **Switch Weapon** | `Q`, `1`, `2`, or Mouse Scroll Wheel | `Y` / Triangle |
| **Dash (Invulnerability)** | `Space` or Left `Shift` | `LT` or `RB` |
| **Use Flask** | `E` | `X` / Square |
| **Pause** | `Esc` | `Start` |

---

## Building & Running Locally

### Prerequisites
- Visual Studio 2022/2026 with "Desktop development with C++" workload (MSVC, CMake) on Windows, or standard GCC/Clang on Linux.
- CMake 3.25+

### Build & Run via Command Line
```bash
# Configure default preset
cmake --preset default

# Build release binary
cmake --build --preset release

# Execute automated test suite (446 tests)
ctest --preset release

# Run the game
build/default/src/Release/HordeCarnage.exe
```

---

## License & Attribution

- **Game Design, Systems Programming & Audio:** Daniel David ([@DanDavD](https://github.com/DanDavD))
- **Typography:** Cinzel by Natanael Gama (SIL Open Font License 1.1)
- **License:** Open source under the [MIT License](LICENSE)
