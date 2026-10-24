# Roadmap

## What it is

The plan to get from an empty repository to a presentable vertical slice in 15 days, from Tuesday October 20 to Tuesday November 3, 2026, at about 8 hours a day. I apply to the internship on Saturday October 31, so the version I present has to be ready that day. The last three days are a buffer.

A vertical slice here means one arena, one boss with two phases, and a full fight from the first attack to "YOU DIED" or "VICTORY". Small, but finished and polished.

## How I build it: in layers

Every layer ends with a game that runs and can be played, and every layer is one short branch. If I stop at any point, `main` is still a working game, just a smaller one.

| Day | Date | Branch | What I build |
|---|---|---|---|
| 1 | Tue Oct 20 | `chore/project-setup` | CMake, raylib, tests, CI, fixed-timestep loop, docs |
| 2 | Wed Oct 21 | `feat/player-movement` | Player entity, keyboard and gamepad movement, arena bounds, camera |
| 3 | Thu Oct 22 | `feat/chaser-enemy` | A hairball enemy that chases the player, circle collisions, contact damage, health bar, game over and restart |
| 4 | Fri Oct 23 | `feat/shooting` | Mouse aim, a pistol and an SMG, pooled bullets (`ObjectPool`), damage, enemy death, hit flash and knockback |
| 5 | Sat Oct 24 | `feat/player-abilities` | A dash with invulnerability and three healing flasks (done early, on day 4) |
| 6 | Sun Oct 25 | `feat/boss-fight` | The Matriarch, its attack state machine (warning, active, recovery), the Charge and the Hair Ring, "YOU DIED" and "VICTORY" |
| 7 | Mon Oct 26 | `feat/boss-patterns` | The Leap with its shadow and shockwave, a seeded random generator, and how the boss picks its next attack |
| 8 | Tue Oct 27 | `feat/boss-shield-and-phase-two` | The alternating shield, the transformation, and phase 2 with the Aimed Volley and the Summon. Release `v0.1.0` |
| 9 | Wed Oct 28 | `feat/data-driven-boss` | The boss's and the guns' numbers in JSON, reloaded with F5 in Debug builds |
| 10 | Thu Oct 29 | `feat/game-feel` | Hit stop, screen shake, particles, sounds, clearer warnings |
| 11 | Fri Oct 30 | `feat/menus-and-polish` | Title screen, pause, the end screens, an art pass with clean shapes, balancing |
| 12 | Sat Oct 31 | `chore/release-v0.2.0` | Bug bash, README GIF, docs. Release `v0.2.0`, the version I present |
| 13-15 | Nov 1-3 | | Buffer |

## Changes to the plan

- **Day 4: guns instead of a melee swing.** I want the feel of twin-stick shooters like SAS: Zombie Assault 4 and Dead Ops Arcade: kiting a horde while shooting behind you. That needs aim independent of movement, so the player aims with the mouse. Because of the change, the `ObjectPool` moves from day 5 to day 4 (bullets need it first), and enemy drops move from day 4 to day 8, where XP gems are the drops.
- **Day 4: one boss fight instead of waves.** With movement, shooting and a dash working, I changed what the game is. The waves, XP, levels, power-ups and new guns I had planned were a long list of small systems, each one shallow. I turned it into the game I wanted to play as a kid: the top-down shooting of Gun Bros with the difficulty of Dark Souls 1. The whole game is one fight against one hard boss, the Matriarch, whose patterns you learn by dying.
  - It asks for fewer systems that go deeper: attacks you can read (a warning, the attack, a recovery to punish), state machines for the attacks, the shield and the phases, a reason to switch guns (a shield that only one of the two guns can break), and a player built to dodge and to commit (a dash with invulnerability, three slow heals).
  - It fits the time better: one enemy that is finished and polished instead of five that are not.
  - Out of the plan: the waves and the wave director, XP and levels, power-ups, new guns, and the charger and the spitter as separate enemies. Their ideas come back as boss attacks: the charge, the spit as a ring of hair, and the hairballs as the boss's summons.
  - The hairball stays, as the boss's summon in phase 2, so its code and its tests are still used. The boss itself reuses its components, its chase input and its movement.
  - The first release waits until the whole fight works (day 8), instead of shipping the waves on day 5.

## Releases

- `v0.1.0` The fight: the boss with its five attacks, its shield and its second phase; the dash and the flasks; "YOU DIED" and "VICTORY".
- `v0.2.0` Vertical slice: numbers in JSON, game feel, menus, balancing. This is the version I present.

## If I fall behind

The fight itself (move, dodge, heal, shoot, die or win) always has to be solid, because it is what shows how I program. I cut in this order:

1. The JSON. The numbers stay in code.
2. The Summon attack of phase 2.
3. The Aimed Volley of phase 2.
4. The sounds.

These never go: the dash, the flasks, the three attacks of phase 1, the shield, phase 2 and the victory screen.

## Architecture goals

These are the pieces I want a reviewer to find in the code, explained in [Architecture](ARCHITECTURE.md) as they land:

- Plain entities with small components, and systems that update them, with clear responsibilities. Not a full data-oriented ECS, which this scope does not need.
- Object pools for bullets, the boss's projectiles, its summons and the particles, so the fight never allocates memory.
- State machines: the boss's phases, each attack (warning, active, recovery), the shield, and the session (playing, dead, victory).
- Data-driven content: the boss's timings and the guns' numbers in JSON, so the difficulty can be tuned without touching code.
- Pure logic separated from rendering and covered by unit tests, including whole fights simulated without a window.
