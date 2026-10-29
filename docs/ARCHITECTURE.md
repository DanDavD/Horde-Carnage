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

When a feature has rules (damage, cooldowns, attack timing), the rules go into `horde_core` with tests. The executable only connects them to the screen and the keyboard.

### The main loop

`Game::Frame` (`src/app/Game.cpp`) does this every frame. On the desktop `Game::Run` calls it in a loop until the window closes; in a browser it hands it to the page, which calls it once per frame (see [The web build](WEB_BUILD.md)):

1. Ask raylib how long the last frame took (`GetFrameTime`).
2. Give that time to `FixedTimestep::Advance`, which answers how many fixed 1/60 s steps are due.
3. Run `Update` that many times, always with the same `dt`. `Update` reads the input (`ReadMoveInput`) and passes it to `StepSession`, which runs all the rules.
4. Draw once, passing `FixedTimestep::Alpha` (how far we are into the next step) so drawing can interpolate between two steps.

Before the steps, `Run` also reads button presses that are events rather than held states (fire, weapon switch, restart, and the Debug overlay toggle), once per frame, so a frame that runs no step cannot lose them.

### Input becomes commands

The simulation never reads a key or the mouse. Every step, `Game::Update` turns the hardware into a `PlayerCommands` (`src/core/PlayerCommands.h`): the movement input, the view-space mouse offset (`aimInView`), twin-stick unit heading (`aimDirection`) or world point (`aimAt`) the player aims with, the fire button, and weapon switches. Tests drive the world with the same struct, and a gamepad fills it without the rules changing. See [Aiming and weapons](WEAPONS.md).

### The world

`World` (`src/core/World.h`) holds everything the simulation needs as plain data: the arena, the player, the boss, the hairballs, the player's bullets, the boss's projectiles and shockwaves, the camera, a seeded random generator (see [Random numbers](RANDOM.md)), what landed the last hit on the player (`lastHitBy`, for the death screen), and the events of the last steps (`events`: hits, breaks, landings, for the game to react to; see [Game feel](GAME_FEEL.md)). Hairballs, projectiles and shockwaves live in fixed-size pools inside it (see [Object pool](OBJECT_POOL.md)), so a running game never allocates memory. Two functions work on it:

- `CreateWorld` builds the starting state: the boss in the middle of the arena, the player south of it and facing it, the camera on both, and the random generator seeded with the seed it is given.
- `StepWorld` advances it by one fixed step, in this order: the invulnerability window and the dash cooldown, the player (start a dash or a drink, the drink, the dash or normal movement), every hairball (chase, movement) and then the boss (`StepBoss`: stalking and picking its next attack from the world's generator, or the stage of its attack and the start of a combo's second attack, the strands of a Hair Ring spawned into their pool when it spits, and the impact and shockwave of a Leap when it lands), collisions between bodies and the walls (four passes, see [Collisions](COLLISIONS.md)), the player's aim and weapons (resolved against `NextFollowCameraCenter` so shots fly straight through the crosshair while strafing), bullets and their hits (see [Bullets and hits](BULLETS.md)), dead hairballs removed, contact damage (see [Health](HEALTH.md)), the boss's strands and shockwaves and their hits on the player (see [The boss](BOSS.md)), then the camera, which follows a point between the player and the boss (see [Camera](CAMERA.md)).

### The session

`Session` (`src/core/Session.h`) wraps the world in a small state machine: `Title` waits over a fresh world, `Playing` steps the world, `Paused` holds it, `Victory` freezes it once the boss is defeated, and `Dead` freezes it once the player's health reaches 0. `TryRestart` replaces the world with a fresh one from `CreateWorld`, with the next seed, and the session keeps the attempt count across it. See [Game flow](GAME_FLOW.md).

`Game` owns one `Session`. It reads the input, calls `StepSession`, and draws the result. It never changes the world itself. After each step it reads the world's events and turns them into a hit stop (which slows the world's clock for a moment), a screen shake (which moves the world's camera), and particles (`ParticleEmitter`, with its own random generator so the fight's seed stays untouched). All three are `horde_core` components that `Game` owns, because they change how the fight is shown, never what happens in it. So are the trails behind the two health bars (`TrailingBar`), which `Game` steps after the world and the HUD draws. See [Game feel](GAME_FEEL.md).

### Drawing

`Game::Draw` only sets up the frame: the black bars and the two cameras (see [Screen and fullscreen](SCREEN.md)). What goes on screen is in `src/app/render/`, one file per part of the picture, each a few free functions that read the world and never change it:

```
src/app/render/
  View          The game's resolution, the screen fit, the world and screen cameras, the mouse in game pixels
  Palette.h     Every colour the game draws with
  Art           The fonts, the textures made in code and the floor's layout, loaded once at the start
  Text          Text in the game's fonts: spaced, centred or aligned to the right
  WorldDraw     DrawWorld: the arena, floor litter, warnings, dust, bodies, projectiles and sparks, in draw order
  ArenaDraw     The stone floor, the walls, the shadows under bodies, the torchlight and the vignette
  ParticleDraw  Floor litter, dust and shadows under bodies, sparks and airborne debris over them
  HairBody      The body of matted hair that hairballs and the boss share (DrawHairBody), and DrawHairball
  Shapes        Shapes raylib does not have: a radius at each angle, a rotated ellipse
  BossDraw      The boss in each pose, its shield, and the warnings of its attacks
  PlayerDraw    The player, the gun, the dash trail and the drink ring
  Hud           The bars, the flasks, the weapon panel, the crosshair, the attempt and the combat prompt strip
  InputPrompts  Procedural keycaps, mouse icons and Xbox/PlayStation/Switch/Generic gamepad prompts
  EndScreen     YOU DIED and VICTORY, with their row of choices
  MenuDraw      The items of each menu, where they sit, the title, pause, settings and controls screens
  Backdrop      The blurred still of the fight behind the pause menu
  DebugOverlay  The F1 overlay, compiled into Debug builds only
```

Each file keeps its own sizes and helpers in an anonymous namespace; only what another file needs is in its header (for example `ShieldColor`, which the boss and the HUD share). `Draw` calls `DrawWorld` inside the world camera, and `DrawVignette`, `DrawHud` and `DrawEndScreen` inside the screen camera. Sound lives next to it, in `src/app/audio/SoundBank`: every sound is synthesized once at start-up and played from the world's events (see [Audio](AUDIO.md)). The boss's theme is in `src/app/audio/MusicPlayer`: composed once at start-up, it reads the session every frame and refills one raylib stream with its layers mixed (see [Music](MUSIC.md)). The player's settings are read before the window opens and saved on every change by `src/app/SettingsStore` (see [Settings](SETTINGS.md)). `Game` owns the one `Art`, loaded after the window opens and unloaded before it closes, and hands it to the drawing. How the game looks, and why, is in [Art direction](ART.md).

### Components and systems

Game objects are plain structs (components) that hold data, and free functions (systems) that update them. The first ones are the `Motion` component and the `StepMovement`, `UpdateFacing` and `ConfineToArena` systems, described in [Movement](MOVEMENT.md). The camera follows the same shape: a `FollowCamera` component and a `StepFollowCamera` system, described in [Camera](CAMERA.md). The app only turns it into a raylib `Camera2D` when it draws. `Player`, `Boss` and `Enemy` are structs of components, not classes with behaviour, and they share the same movement systems (see [Enemies](ENEMIES.md) and [The boss](BOSS.md)). The boss's attacks are a small state machine in plain data: the attack in progress and its stage (warning, active, recovery), advanced by `StepBoss`. The boss never touches the world's pools: `StepBoss` returns what the world has to do in that step (`BossActions`), such as spawning a ring of hair, and `StepWorld` does it. `horde_core` has its own `Vec2` type, so it never includes raylib; the app converts with `ToRaylib` right before drawing.

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
- **Drawing split by part of the picture, as free functions.** All of it used to be in `Game.cpp`, which had grown past 1200 lines, so changing the look of the boss meant scrolling past the HUD and the debug overlay. Now the art of each part can change in its own file, and `Game.cpp` is the loop, the input and a short `Draw`.
  - Rejected: a renderer class or a draw method on each entity. The entities are plain data in `horde_core`, which must not know about raylib, and a class would only hold functions that need no state of their own.
- **Warnings as errors only for my code.** `horde_set_warnings` adds `/W4 /WX` to the project's targets. raylib and doctest are added as `SYSTEM`, so a warning in their headers cannot break my build.

## How to tune it

- The simulation rate and the frame clamp are the two constructor arguments of `FixedTimestep` (`1.0 / 60.0` and `0.25` by default). `Game` uses the defaults.
- The game's resolution is `kScreenWidth` and `kScreenHeight` in `src/app/render/View.h`.
- The colours are all in `src/app/render/Palette.h`; the sizes of each drawing are at the top of its own file in `src/app/render/`.

## Tests

- `tests/core/FixedTimestepTests.cpp`: exact steps, accumulation of short frames, the remainder of long frames (`Alpha`), the clamp on huge frames, negative frame times and the 60 Hz default.
- `tests/core/Vec2Tests.cpp`, `MovementTests.cpp`, `FacingTests.cpp`, `ArenaTests.cpp`, `AnalogInputTests.cpp` and `GamepadAimTests.cpp`: see [Movement](MOVEMENT.md#tests) and [Controllers](CONTROLLERS.md#tests).
- `tests/core/FollowCameraTests.cpp`: see [Camera](CAMERA.md#tests).
- `tests/core/ChaseTests.cpp` and `WorldTests.cpp`: see [Enemies](ENEMIES.md#tests).
- `tests/core/AttackChoiceTests.cpp`, `BossTests.cpp`, `BossChargeTests.cpp`, `BossComboTests.cpp`, `BossHairRingTests.cpp`, `BossLeapTests.cpp`, `BossVolleyTests.cpp`, `FrenzyTests.cpp`, `PhaseTwoTests.cpp`, `ShockwaveTests.cpp` and `SummonTests.cpp`: see [The boss](BOSS.md#tests). The boss tests share `PredictableBoss.h`, a boss without its chance.
- `tests/core/RandomTests.cpp`: see [Random numbers](RANDOM.md#tests).
- `tests/core/WorldEventTests.cpp`, `HitStopTests.cpp`, `ScreenShakeTests.cpp`, `ImpactFeelTests.cpp`, `ParticleTests.cpp`, `ParticleEmitterTests.cpp` and `TrailingBarTests.cpp`: see [Game feel](GAME_FEEL.md#tests).
- `tests/core/SynthTests.cpp` and `SoundCueTests.cpp`: see [Audio](AUDIO.md#tests).
- `tests/core/SequencerTests.cpp`, `BossThemeTests.cpp`, `MusicDirectorTests.cpp` and `MusicMixerTests.cpp`: see [Music](MUSIC.md#tests).
- `tests/core/ScreenFitTests.cpp`: see [Screen and fullscreen](SCREEN.md#tests).
- `tests/core/StoneFloorTests.cpp`: see [Art direction](ART.md#tests).
- `tests/core/ShieldTests.cpp`: see [The boss's shield](SHIELD.md#tests).
- `tests/core/CollisionTests.cpp` and the crowd cases in `WorldTests.cpp`: see [Collisions](COLLISIONS.md#tests).
- `tests/core/HealthTests.cpp` and the contact damage cases in `WorldTests.cpp`: see [Health](HEALTH.md#tests).
- `tests/core/SessionTests.cpp`: see [Game flow](GAME_FLOW.md#tests).
- `tests/core/MenuTests.cpp`, `MenuRepeatTests.cpp`, `GamepadConnectionTests.cpp` and `GamepadTypeTests.cpp`: see [Menus](MENUS.md#tests) and [Controllers](CONTROLLERS.md#tests).
- `tests/core/SettingsTests.cpp`: see [Settings](SETTINGS.md#tests).
- `tests/core/WeaponTests.cpp`, `LoadoutTests.cpp`, `LaserSightTests.cpp` and the aim and fire cases in `FacingTests.cpp` and `WorldTests.cpp`: see [Aiming and weapons](WEAPONS.md#tests).
- `tests/core/ObjectPoolTests.cpp`: see [Object pool](OBJECT_POOL.md#tests).
- `tests/core/DashTests.cpp`, `FlasksTests.cpp` and the dash and flask cases in `WorldTests.cpp`: see [Player abilities](ABILITIES.md#tests).
- `tests/core/ProjectileTests.cpp`, the sweep cases in `CollisionTests.cpp` and the bullet cases in `WorldTests.cpp`: see [Bullets and hits](BULLETS.md#tests).

## Limitations

- I only build and test on Windows with MSVC. The CMake setup should work with GCC and Clang, but I have not tried it.
