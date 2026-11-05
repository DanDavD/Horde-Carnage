# Horde Carnage

A top-down boss fight in C++20 on raylib: the top-down shooting of Gun Bros with the difficulty of Dark Souls. One arena, one very hard boss, and you learn her patterns by dying.

[![CI](https://github.com/DanDavD/Horde-Carnage/actions/workflows/ci.yml/badge.svg)](https://github.com/DanDavD/Horde-Carnage/actions/workflows/ci.yml)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)
![raylib](https://img.shields.io/badge/raylib-5.5-black)
![Status](https://img.shields.io/badge/status-v1.0.0-brightgreen)
[![Roadmap on Trello](https://img.shields.io/badge/roadmap-Trello-0052CC?logo=trello)](https://trello.com/b/4Ii5Aoep/horde-carnage-roadmap)

[![Gameplay video](https://img.youtube.com/vi/Oz9MaF9-lhU/maxresdefault.jpg)](https://youtu.be/Oz9MaF9-lhU)

**[Play in the browser on itch.io](https://dandavd.itch.io/horde-carnage-demo)** | **[Download for Windows](https://github.com/DanDavD/Horde-Carnage/releases/latest)** | **[Watch the gameplay video](https://youtu.be/Oz9MaF9-lhU)** | **[Roadmap on Trello](https://trello.com/b/4Ii5Aoep/horde-carnage-roadmap)**

## In short

- **Solo project**, planned and shipped like a small team would: branches, pull requests, CI and a changelog.
- **No engine.** raylib only gives me a window, input, 2D drawing and audio output. The loop, entities, collisions, AI, art, sound and music are my own code.
- **Over 440 unit tests**, warnings as errors (`/W4 /WX`), and CI that builds and tests every pull request on Windows and builds the WebAssembly version.

## Engineering highlights

- **Boss AI from layered state machines.** Phases (one, transforming, two, frenzy), attacks (warning, active, recovery) and an alternating shield each run their own state machine. Five telegraphed attacks, picked at random and weighted by distance, never the same twice in a row, with combos. See [The boss](docs/BOSS.md) and [The shield](docs/SHIELD.md).
- **Deterministic simulation.** A fixed 60 Hz step with interpolated rendering, and every random choice from a seeded PCG32 generator, so the same seed and inputs always give the same fight. Particles and sounds draw from their own streams, so cosmetics never change the fight. See [Random numbers](docs/RANDOM.md).
- **Pure logic, tested without a window.** All the rules live in `horde_core`, a library that never opens a window; the app only turns input into commands and draws the world. That is what makes the tests possible. See [Architecture](docs/ARCHITECTURE.md).
- **Fixed-size object pools** for bullets, the boss's strands, shockwaves, summon marks, hairballs, events and 512 particles, so no gameplay object is allocated during a fight. See [Object pool](docs/OBJECT_POOL.md).
- **Everything procedural, no image or audio files.** The art is drawn with shapes in code, the 25 sound effects are synthesized from recipes, and the music is a 5-layer metal theme, also synthesized, that follows the boss's phase and your health. See [Art](docs/ART.md), [Audio](docs/AUDIO.md) and [Music](docs/MUSIC.md).
- **The same game in the browser**, compiled to WebAssembly with Emscripten. Getting there meant fixing a crash where the game object overflowed the browser's 64 KB stack into raylib's globals. See [The web build](docs/WEB_BUILD.md).

## What is in the game

- **The Matriarch**, a giant hairball with five telegraphed attacks (Charge, Hair Ring, Leap, Aimed Volley, Summon), combos, a second phase and a frenzy at half of her second bar.
- **An alternating shield** that each gun breaks differently: the pistol cracks the hard Shell, only the SMG's stream outpaces the regenerating Barrier.
- **The hunter**: twin-stick aiming with a laser sight, a pistol and an SMG, a dash that makes you untouchable, and three Dark Souls style flasks.
- **Game feel**: eased slow motion on heavy hits, screen shake, sparks, casings, dust and hair from a 512-particle pool, trailing health bars, and a boss that dies on screen.
- **Menus and platforms**: title, pause and settings (volumes, sensitivity, screen shake, display, frame rate); keyboard and mouse or Xbox, PlayStation and Switch gamepads with aim assist and prompts drawn in code; native-resolution fullscreen; Windows and the browser.

## Getting started

You need Visual Studio 2026 or 2022 with the **Desktop development with C++** workload (it includes MSVC, the Windows SDK and CMake), and an internet connection for the first configure, which downloads raylib, doctest and the Cinzel font at pinned versions. From a *Developer PowerShell for VS*:

```
cmake --preset default
cmake --build --preset release
ctest --preset release
build\default\src\Release\HordeCarnage.exe
```

`--preset debug` keeps a console with raylib's log and enables the Debug shortcuts. The web build is described in [The web build](docs/WEB_BUILD.md).

| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Move | `WASD` or the arrow keys | Left stick |
| Aim and fire | Mouse, left click (hold with the SMG) | Right stick, `RT` |
| Switch gun | `Q`, `1`, `2` or the mouse wheel | `Y` / Triangle |
| Dash | `Space` or left `Shift` | `LT` or `RB` |
| Drink a flask | `E` | `X` / Square |
| Pause | `Esc` | `Start` |

`R` retries at once after a fight ends, and `--windowed` opens one run in a window. Every control, the menus included, is listed in the game under *Settings > Controls*, and the gamepad is explained in [Controllers](docs/CONTROLLERS.md). Debug builds add `F1` (overlay), `F2` (break the shield), `F3` (skip to the second phase, then the frenzy) and `F4` (defeat the boss).

## Project structure

```
src/core/      Pure game logic, no window (library horde_core)
src/app/       Window, main loop, input, drawing and audio (executable HordeCarnage)
tests/core/    Unit tests for src/core
web/           The page around the browser build
docs/          One document per system: how it works, decisions, tuning, tests, limits
```

## Roadmap

- [x] `v0.1.0` The whole fight: five attacks, shield, second phase, frenzy, dash and flasks
- [x] `v1.0.0` Art, game feel, sound, music, menus, settings, gamepads and the browser build
- [ ] `v1.1.0` A bar as the hub (shop, trophies, collection), a map of the hunt between bosses, and a second boss, The Tolling Bell, whose attacks land on the beat of her music

I plan the work card by card on the public [Trello board](https://trello.com/b/4Ii5Aoep/horde-carnage-roadmap). The history of v1.0.0 is in the [roadmap](docs/ROADMAP.md) and the [changelog](CHANGELOG.md), and how I work is in [CONTRIBUTING.md](CONTRIBUTING.md).

## Credits

The Cinzel typeface is by Natanael Gama and the Cinzel Project Authors, under the SIL Open Font License 1.1, shipped next to the game as `Cinzel-OFL.txt`. Everything else, the art included, is made in code in this repository.
