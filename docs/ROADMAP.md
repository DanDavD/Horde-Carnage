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
| 7 | Mon Oct 26 | `feat/boss-patterns` | The Leap with its shadow and shockwave, a seeded random generator, how the boss picks its next attack, and combos |
| 8 | Tue Oct 27 | `feat/boss-shield-and-phase-two` | The alternating shield, the transformation, and phase 2 with the Aimed Volley and the Summon. Release `v0.1.0` |
| 9 | Wed Oct 28 | `feat/data-driven-boss` | The boss's and the guns' numbers in JSON, reloaded with F5 in Debug builds |
| 10 | Thu Oct 29 | `feat/game-feel` | Hit stop, screen shake, particles, sounds, clearer warnings |
| 11 | Fri Oct 30 | `feat/menus-and-polish` | Title screen, pause, the end screens, an art pass with clean shapes, balancing |
| 12 | Sat Oct 31 | `chore/release-v1.0.0` | Bug bash, README GIF, docs. Release `v1.0.0`, the version I present |
| 13-15 | Nov 1-3 | | Buffer |

## Changes to the plan

- **Day 4: guns instead of a melee swing.** I want the feel of twin-stick shooters like SAS: Zombie Assault 4 and Dead Ops Arcade: kiting a horde while shooting behind you. That needs aim independent of movement, so the player aims with the mouse. Because of the change, the `ObjectPool` moves from day 5 to day 4 (bullets need it first), and enemy drops move from day 4 to day 8, where XP gems are the drops.
- **Day 4: one boss fight instead of waves.** With movement, shooting and a dash working, I changed what the game is. The waves, XP, levels, power-ups and new guns I had planned were a long list of small systems, each one shallow. I turned it into the game I wanted to play as a kid: the top-down shooting of Gun Bros with the difficulty of Dark Souls 1. The whole game is one fight against one hard boss, the Matriarch, whose patterns you learn by dying.
  - It asks for fewer systems that go deeper: attacks you can read (a warning, the attack, a recovery to punish), state machines for the attacks, the shield and the phases, a reason to switch guns (a shield that only one of the two guns can break), and a player built to dodge and to commit (a dash with invulnerability, three slow heals).
  - It fits the time better: one enemy that is finished and polished instead of five that are not.
  - Out of the plan: the waves and the wave director, XP and levels, power-ups, new guns, and the charger and the spitter as separate enemies. Their ideas come back as boss attacks: the charge, the spit as a ring of hair, and the hairballs as the boss's summons.
  - The hairball stays, as the boss's summon in phase 2, so its code and its tests are still used. The boss itself reuses its components, its chase input and its movement.
  - The first release waits until the whole fight works (day 8), instead of shipping the waves on day 5.
- **Day 8: a frenzy at half of the second bar.** Not in the plan. Playing it, fighting from far away was too safe and the second half of phase 2 felt like the first, so at half health the boss falls into a frenzy and each attack gains a twist: strands thrown during the Charge, two Hair Rings, a ring of strands on landing, five Volley bursts. See [The boss](BOSS.md).
- **Day 7: the art starts early, with the arena, the player and the HUD.** The plan had an art pass with clean shapes on day 11. With the fight done early, I started it on day 7 and gave it a direction: the readable shapes of Brotato in the dark tone of Dark Souls, all drawn in code. A stone crypt lit by the player's torch, a hunter in a tricorn hat, and a HUD in the Cinzel typeface. The boss and the hairballs follow. See [Art direction](ART.md).
- **Day 8: the Barrier caps every hit.** The plan said the orange Barrier would only grow back fast enough that the pistol could not keep up. At full rate the pistol does 60 a second, the same as the SMG, so that rule alone would let a fast clicker break it with either gun. Each hit on the Barrier now takes at most 6, which leaves the pistol at 36 a second against the 40 it grows back. See [The boss's shield](SHIELD.md#design-decisions).

## The difficulty I am aiming for

A player who has never seen the fight should need about 10 to 15 attempts to win it. Two attacks that always come in the same order, as on day 6, are far from that. The difficulty comes from how the pieces combine, and I build it in this order:

- **Day 7, patterns.** The Leap. A seeded random choice of attack, weighted by distance and never the same twice in a row, so the order cannot be memorised. Combos with no rest between attacks: two Charges in a row, a Leap after a Charge, a ring on landing from the Leap. Warnings of varying length, so a dash timed to a rhythm fails. Less stalking between attacks, 0.8 to 1.6 s instead of 2. The death screen names the attack that killed. I dropped the Hair Ring into a Charge from the first idea: the run would be aimed while the player is still threading the strands, with no room to step out of its line.
- **Day 8, shield and phase 2.** The shield forces a weapon switch in the middle of the attacks. Phase 2 has shorter warnings, the Aimed Volley, the Summon, and rings with two waves or a gap that turns. The bar refills, so phase 2 is a second fight.
- **Day 9, tuning.** With the numbers in JSON and F5 to reload them, I tune damage, timings and health while playing.
- **Day 11, balance.** Playtests against these targets: a clean win lasts about 3 to 4 minutes (phase 1 about 1.5, phase 2 about 2), the player survives 4 or 5 hits (about 8 with the flasks), and each attack takes 1 or 2 deaths to learn.

Hard, but fair: no hit lands without a visible warning. A death that does not show what killed the player is a design bug, so the "YOU DIED" screen will name the attack that landed the last hit.

Someone reviewing the game may only play it for a few minutes and never reach phase 2. The README will show phase 2 in a GIF, and on day 11 I decide whether the game also needs a practice option.

## Releases

- `v0.1.0` The fight: the boss with its five attacks, its shield, its second phase and its frenzy; the dash and the flasks; "YOU DIED" and "VICTORY". Released on October 25.
- `v1.0.0` Vertical slice: art, game feel, sound, menus, balancing. This is the version I present, and the first one that is not a pre-release.

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
