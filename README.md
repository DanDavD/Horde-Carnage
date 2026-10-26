# Horde Carnage

A top-down boss fight, written in C++20 with raylib.

One arena, one very hard boss: the Matriarch, a giant hairball. You run with the keyboard, aim and shoot with the mouse, dash through its attacks and drink one of your three flasks when you dare. You learn its patterns by dying.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)
![raylib](https://img.shields.io/badge/raylib-5.5-black)
![Status](https://img.shields.io/badge/status-in%20development-orange)

**Play it:** download the latest zip from [Releases](https://github.com/DanDavD/Horde-Carnage/releases), unzip it and run `HordeCarnage.exe` (Windows 10 or 11, nothing to install).

---

## The idea

I'm building Horde Carnage as a vertical slice in 15 days (October 20 to November 3, 2026). It started as a horde shooter inspired by Dungeon Rampage, The Binding of Isaac and Castle Crashers, with the twin-stick shooting of SAS: Zombie Assault 4 and the "Dead Ops Arcade" mode of Call of Duty: Black Ops. On day 4 I turned it into the game I wanted to play as a kid: the top-down shooting of Gun Bros with the difficulty of Dark Souls 1. The whole game is one fight, with telegraphed attacks, a second phase, a dash that makes you untouchable and only three heals. The reasons are in the [roadmap](docs/ROADMAP.md#changes-to-the-plan).

I chose plain C++ with a small library instead of an engine on purpose. raylib only gives me a window, input, 2D drawing and audio. Everything that makes it a game (the loop, entities, collisions, the boss and its attacks, the player's abilities) is my own code, and that is the part I want to show.

## Current status

This is day 7 of 15, and the boss fight is in progress. Progress is tracked in the [roadmap](docs/ROADMAP.md) and the [changelog](CHANGELOG.md).

**What works today:**
- A CMake project that downloads raylib and doctest at a pinned version, so a fresh clone builds with one command.
- A window with a fixed-timestep main loop: the game simulates at 60 Hz whatever the monitor's refresh rate, and renders once per frame.
- A player you move around an arena with the keyboard or a gamepad. It speeds up and stops over a fraction of a second, turns back quickly, is never faster on a diagonal, faces the way you push, and slides along the walls. See [Movement](docs/MOVEMENT.md).
- An arena bigger than the screen, with a camera that follows the player smoothly, looks a little ahead of where you run, leans toward the boss so you can see it, and stops at the walls. See [Camera](docs/CAMERA.md).
- The Matriarch, a boss three times the size of a hairball, waiting in the middle of the arena. It walks at you slower than you run, shoves you out of its way, takes 20 hit points per touch, and needs 120 pistol bullets to go down. Its health bar runs along the bottom of the screen. See [The boss](docs/BOSS.md).
- Its first attack, the Charge. It crouches and draws its path on the floor; the path follows you, then locks 0.3 s before it runs at 1000 units/s. A hit takes 30 and throws you aside. If the run ends at a wall, it is stunned for 1.5 s. See [The boss](docs/BOSS.md).
- Its second attack, the Hair Ring. It swells for 0.6 s while a red ring and a pale lane on the floor show where its hair will fly and where the gap is, always off to one side of you. Then it spits 42 slow strands in every direction but the gap, and each one that hits takes 15. Run to the gap or dash through the ring. See [The boss](docs/BOSS.md).
- Its third attack, the Leap. It crouches, jumps out of reach and comes down on its shadow, which follows you, slower than you run, then stops a moment before the landing. The landing takes 35, and a shockwave rolls out from it faster than you can run: dash through it. See [The boss](docs/BOSS.md).
- A boss that picks its next attack at random, weighted by how far you are: up close it favours the Hair Ring, far away the Charge, in between the Leap, and it never repeats the attack it just did. It stalks for 0.8 to 1.6 s between attacks, and every warning runs up to 15% longer or shorter, so you have to watch it instead of counting. The moment its aim or its shadow locks always lasts the same. See [The boss](docs/BOSS.md).
- Combos. About a third of the Charges and Leaps chain straight into a second attack with a shorter warning and no opening in between: the Rebound (a second Charge back off the wall), the Pursuit (a Leap after you once the Charge is over) and the Crash (a Hair Ring on landing). The opening after a combo is longer. See [The boss](docs/BOSS.md).
- An alternating shield. The blue Shell shrugs off bullets under 8 damage, so only the pistol breaks it; the orange Barrier takes at most 6 per bullet and grows back, so only the SMG's stream breaks it. A break staggers the boss and leaves it open to both guns for 5 s, then the other shield comes up. Its bar runs under the boss's. See [The boss's shield](docs/SHIELD.md).
- A second phase. Emptying the first bar starts a 2 s transformation: the boss swells and roars, its bar fills back up, and a short shockwave knocks back anyone waiting next to it. Then its eyes turn red, it stalks less, combos more, its shield opens for 4 s instead of 5, and it has a fourth attack, the Aimed Volley: three quick bursts of five strands, the first and last aimed where you are and the middle one where you are running to. At half of its second bar it falls into a frenzy: it throws strands at you while it charges, spits two rings, lands from a Leap with a ring of strands, and fires five bursts. Only the second bar wins. See [The boss](docs/BOSS.md).
- A seeded random number generator (PCG32) behind all of it, with a new seed for every attempt, so a fight can be replayed exactly from its seed. See [Random numbers](docs/RANDOM.md).
- The hairball, the boss's small child: it walks straight at you and stops when it touches you. The boss summons them in its second phase: marks appear on the floor around you, and 0.75 s later a hairball comes out of each, so keeping far from the boss is no longer safe. Summoned hairballs spit a strand at you every 3 s, each on its own beat, and swell red just before. See [Enemies](docs/ENEMIES.md).
- Bodies push each other apart instead of stacking. You can shove a hairball out of your way but not the boss, and a crowd slows you down. See [Collisions](docs/COLLISIONS.md).
- A health bar. A touch of the boss takes 20 of your 100 hit points (a hairball's takes 10), and after a hit you blink and cannot be hurt for 0.8 seconds. See [Health](docs/HEALTH.md).
- A fight that ends one of two ways. "YOU DIED" shows your attempt number, what killed you and when, and how much health the boss kept; "VICTORY" shows the attempt that won, the length of the fight and your accuracy. Both fade in, then a button starts a fresh fight. The attempt count survives every retry. See [Game flow](docs/GAME_FLOW.md).
- Aiming with the mouse, independent of where you walk, and two guns: a semi-automatic pistol (one shot per click) and an automatic SMG (hold to fire). A panel shows the gun in hand. See [Aiming and weapons](docs/WEAPONS.md).
- Bullets that fly and hit the first body in their path. A hairball flashes white and gets knocked back; the boss only flashes, it is too heavy to push. See [Bullets and hits](docs/BULLETS.md).
- Hairballs, bullets, the boss's strands, its shockwaves and the marks of its Summon live in fixed object pools, so the game never allocates memory while you play. See [Object pool](docs/OBJECT_POOL.md).
- A dash that covers about four body widths in 0.12 s, passes through enemies and makes you untouchable while it lasts. See [Player abilities](docs/ABILITIES.md).
- Three healing flasks per attempt, Dark Souls style: drinking takes a second, slows you down, and a hit while drinking wastes the flask. See [Player abilities](docs/ABILITIES.md).
- Unit tests for the pure logic, and a GitHub Actions workflow that builds and tests every push and pull request on Windows, and builds the web version.
- A web build: the same game compiled to WebAssembly, playable in a browser tab. See [The web build](docs/WEB_BUILD.md).
- A dark look drawn entirely in code: a crypt of cracked stone slabs and black walls, lit only by the torch you carry, with shadows that lean away from it; a hunter in a tricorn hat and a torn cape, both hands on the gun, the other gun slung or holstered; and a HUD of thin bars in the Cinzel typeface. See [Art direction](docs/ART.md).
- Always fullscreen, at the monitor's own resolution and refresh rate, drawn sharp at that resolution, with black bars on screens that are not 16:9 so everyone sees the same arena. `--windowed` opens a window instead. See [Screen and fullscreen](docs/SCREEN.md).
- Warnings are errors for the project's own code (`/W4 /WX` on MSVC).

**Still to do:** the boss and the hairballs in the new look, game feel, sound and menus, and maybe the numbers in JSON. See the [roadmap](docs/ROADMAP.md) for the order, and for the difficulty I am aiming for.

## Tech stack

| Area | Choice |
|---|---|
| Language | C++20 |
| Window, input, 2D rendering, audio | [raylib](https://www.raylib.com/) 5.5 |
| Build | CMake 3.25+ with presets |
| Tests | [doctest](https://github.com/doctest/doctest) 2.4.11 |
| Web | [Emscripten](https://emscripten.org/) 3.1.74, WebAssembly |
| CI | GitHub Actions (Windows with MSVC, and the web build) |
| Font | [Cinzel](https://github.com/NDISCOVER/Cinzel) by Natanael Gama, SIL Open Font License 1.1 |

## Getting started

**You need:**
- Windows: Visual Studio 2026 (or 2022) with the **Desktop development with C++** workload. It includes MSVC, the Windows SDK and CMake.
- Other platforms: any C++20 compiler and CMake 3.25 or newer. I only test on Windows for now.
- An internet connection for the first configure, which downloads raylib, doctest and the Cinzel font.

From a *Developer PowerShell for VS*, in the repository folder:

```
cmake --preset default
cmake --build --preset release
build\default\src\Release\HordeCarnage.exe
```

You can also open the folder in Visual Studio (*File > Open > Folder*). It reads `CMakePresets.json` by itself.

**Controls:**

| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Move | `WASD` or the arrow keys | Left stick or d-pad |
| Aim | Mouse | Not yet |
| Fire | Left click (hold with the SMG) | Not yet |
| Switch gun | `Q`, `1`, `2` or the mouse wheel | Not yet |
| Dash | `Space` or left `Shift` | Not yet |
| Drink a flask | `E` | Not yet |
| Try again after "YOU DIED" or "VICTORY" | `Enter` or `R` | `A` or `Start` |
| Quit | `Esc` | |
| Play in a window instead of fullscreen | Start it with `--windowed` | |
| Debug overlay (Debug builds only) | `F1` | |
| Break the boss's shield (Debug builds only) | `F2` | |
| Skip to the second phase, then to the frenzy (Debug builds only) | `F3` | |
| Defeat the boss at once (Debug builds only) | `F4` | |

## Building and testing

- **Debug build:** `cmake --build --preset debug`. It keeps a console window with raylib's log.
- **Release build:** `cmake --build --preset release`. A plain windowed app, no console.
- **Tests:** `ctest --preset release` (or `--preset debug`). The tests only cover code in `horde_core`, which never opens a window.
- **Web build:** with the Emscripten SDK activated, `cmake --preset web` then `cmake --build --preset web`. It writes `index.html`, `index.js` and `index.wasm` to `build/web/src`, the page I publish on itch.io. See [The web build](docs/WEB_BUILD.md).

## Project structure

```
CMakeLists.txt        Project root: C++ standard, options, dependencies
CMakePresets.json     Configure, build and test presets
cmake/
  Dependencies.cmake  raylib, doctest and the Cinzel font, pinned with FetchContent
  EmbedFile.cmake     Compiles the font files into the executable
  Warnings.cmake      Strict warnings for the project's own targets
src/
  core/               Pure game logic, no window (library horde_core)
  app/                The game: window, main loop, input (executable HordeCarnage)
    render/           Drawing, one file per part of the picture
  main.cpp
tests/
  core/               Unit tests for src/core
web/                  The page around the browser build
docs/                 Design and architecture notes
```

See [Architecture](docs/ARCHITECTURE.md) for how the pieces fit together.

## Roadmap

The full day-by-day plan, with what I cut first if I fall behind, is in [docs/ROADMAP.md](docs/ROADMAP.md).

- [x] `v0.0.1` Project setup: CMake, raylib, tests, CI, fixed-timestep loop
- [x] `v0.1.0` The fight: the boss with its five attacks, its shield, its second phase and its frenzy, the dash and the flasks, death and victory
- [ ] `v1.0.0` The version I present: art, game feel, sound, menus and balancing

## Credits

- The Cinzel typeface is by Natanael Gama and the Cinzel Project Authors, under the SIL Open Font License 1.1. The license ships next to the game as `Cinzel-OFL.txt`.
- Everything else, the art included, is drawn in code in this repository.

## Contributing

This is a solo project, but I work on it like a small team would. See [CONTRIBUTING.md](CONTRIBUTING.md).
