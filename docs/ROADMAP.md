# Roadmap

## What it is

The plan to get from an empty repository to a presentable vertical slice in 15 days, from Tuesday October 20 to Tuesday November 3, 2026, at about 8 hours a day.

A vertical slice here means one arena, a few enemy types, a full run from the first wave to the boss, and a win and a lose screen. Small, but finished and polished.

## How I build it: in layers

Every layer ends with a game that runs and can be played, and every layer is one or two short branches. If I stop at any point, `main` is still a working game, just a smaller one.

| Day | Date | Branch | What I build |
|---|---|---|---|
| 1 | Tue Oct 20 | `chore/project-setup` | CMake, raylib, tests, CI, fixed-timestep loop, docs |
| 2 | Wed Oct 21 | `feat/player-movement` | Player entity, keyboard and gamepad movement, arena bounds, camera |
| 3 | Thu Oct 22 | `feat/chaser-enemy` | A hairball enemy that chases the player, circle collisions, contact damage, health bar, game over and restart |
| 4 | Fri Oct 23 | `feat/melee-combat` | Swing attack with an arc hitbox, damage, knockback, hit flash, enemy death and drops |
| 5 | Sat Oct 24 | `feat/enemy-waves` | `ObjectPool` for enemies, spawner and wave director. Release `v0.1.0` |
| 6 | Sun Oct 25 | `feat/data-driven-content` | Enemy, weapon and wave stats loaded from JSON files instead of code |
| 7 | Mon Oct 26 | `feat/enemy-state-machine` | Enemy state machine, a charger and a spitter, pooled projectiles |
| 8 | Tue Oct 27 | `feat/xp-and-level-up` | XP gems, levels, a level-up screen with three choices |
| 9 | Wed Oct 28 | `feat/weapons` | Two or three weapons and their upgrades. Release `v0.2.0` |
| 10 | Thu Oct 29 | `feat/power-ups` | Pickups in the arena (heal, speed, rage). Checkpoint: decide what to cut |
| 11 | Fri Oct 30 | `feat/boss` | Boss with its own state machine and two or three attack patterns, boss health bar, win screen |
| 12 | Sat Oct 31 | `feat/game-feel` | Hit stop, screen shake, particles, sounds, main menu, pause |
| 13 | Sun Nov 1 | `feat/art-pass` | Final look of the arena, the player and the enemies |
| 14 | Mon Nov 2 | `chore/release-v0.3.0` | Bug bash, balancing, release build, README GIF, docs. Release `v0.3.0` |
| 15 | Tue Nov 3 | | Buffer for anything that slipped, then present |

## Releases

- `v0.1.0` Core loop: move, fight, survive waves, die.
- `v0.2.0` Progression: level up and change your weapon.
- `v0.3.0` Vertical slice: power-ups, boss, game feel. This is the version I present.

## If I fall behind

The core loop (move, fight, waves, die or win) always has to be solid, because it is what shows how I program. On day 10 I check where I am, and I cut in this order:

1. The boss gets simpler: one or two patterns instead of three.
2. Fewer weapons: one weapon with upgrades instead of several.
3. Power-ups go away.
4. The art pass becomes clean shapes and colours instead of sprites.

## Architecture goals

These are the pieces I want a reviewer to find in the code, explained in [Architecture](ARCHITECTURE.md) as they land:

- Plain entities with small components, and systems that update them, with clear responsibilities. Not a full data-oriented ECS, which this scope does not need.
- Object pools for enemies, projectiles and pickups, so a big wave does not allocate memory every frame.
- State machines for enemy and boss behaviour.
- Data-driven content: enemy, weapon and wave numbers in JSON, so they can be tuned without touching code.
- Pure logic separated from rendering and covered by unit tests.
