# The boss

## What it is

The Matriarch, the one boss the game is about: a giant hairball, mother of the small ones, in the middle of the arena. The whole game is one fight against it, in the spirit of Gun Bros and Dark Souls 1: a boss that is hard on purpose, whose patterns you learn by dying. See the [roadmap](ROADMAP.md#changes-to-the-plan) for why.

This document describes what exists today. The boss walks at the player, shoves them, hurts on touch and soaks up bullets. Its attacks, its shield and its second phase come in the next steps of the roadmap.

The rules live in `horde_core`:

- `src/core/Boss.h`: the `Boss` struct, with the same body components as the player and the hairballs (`Motion`, `MovementStats`, a radius and a weight) and the boss's numbers.
- `src/core/World.h`, `src/core/World.cpp`: `kBossArena` and `kStartingDistance`; `CreateWorld` places the boss; `StepBoss` moves it; the collision, bullet and contact damage systems include it; and `CameraFocus` keeps it in view.

The game executable draws it:

- `src/app/Game.cpp`: `DrawBoss` draws the same hairy body as a hairball (`DrawHairBody`), three times bigger, darker, with glowing eyes. `DrawBossBar` draws its health along the bottom of the screen, under its name.

## How it works

**The start.** `CreateWorld` puts the boss in the middle of the arena (1640x1040) and the player `kStartingDistance` (400 units) south of it, facing it. There are no hairballs.

**Every fixed step,** after the player and the hairballs have moved:

1. **Chase.** `StepBoss` gives the boss the same `ChaseInput` as a hairball: straight at the player, stopping at contact. It moves through `StepMovement` with its own stats: 120 units/s at most, slow to get going (400 units/s^2).
2. **Collide.** The collision passes treat the boss as one more body (see [Collisions](COLLISIONS.md)). At weight 40 against the player's 4, the player takes 91% of any overlap, so the boss shoves the player and the player cannot shove it. A dashing player passes through it.
3. **Take bullets.** For each bullet, `FirstHit` sweeps it against every hairball and the boss, and the body it touches first takes it. `HitBoss` takes the bullet's damage and starts a 0.1 s flash. No stun and no knockback.
4. **Hurt on touch.** `ApplyContactDamage` checks the boss before the hairballs. A touch takes 20 through `HurtPlayer`, the single door for damage (see [Health](HEALTH.md)).

**The camera** follows `CameraFocus`: the player, pulled 40% of the way toward the boss, but never more than 200 units. See [Camera](CAMERA.md).

**Death.** At 0 health the boss is out of the simulation: `StepBoss` brakes it to a stop, it is no longer a body in the collisions, bullets fly through it, it does not hurt, and the camera goes back to the player. It is drawn grey with dark eyes. Nothing else happens yet: the victory screen is the next step.

## Design decisions

- **A type of its own, not an `Enemy` with bigger numbers.** The boss shares every body system with the hairballs, but its rules differ: bullets neither stagger nor push it, it will have attacks with a warning, an active part and a recovery, and it will change phase. Putting that into `Enemy` would give every hairball fields it never uses, and a flag checked everywhere.
  - Rejected: a boss flag in `Enemy`. Every system would ask "is this the boss?", and the hairball tests would carry the boss's rules.
  - Rejected: a base class with virtual behaviour. As with the hairballs, the difference is in what the body decides to do, not in how it moves, so it belongs in data and systems.
- **Exactly one boss, as a plain member of `World`.** The game is one fight. A pool or a list for one object would only add loops and empty checks.
- **A dead boss is inert, not removed.** The `World` always has its boss, so no system needs to ask whether there is one. The tests about the player and the hairballs use a world with a dead boss for the same reason: it is the boss "not being there" by the game's own rules.
- **No stagger and no knockback from bullets.** The SMG hits ten times a second. If each bullet staggered the boss, the SMG would lock it in place, and the fight would be about holding the button. The openings to punish it will come from its own attacks (the recovery after each one) and from its shield.
- **Heavy, but not immovable.** Weight 40 means the boss wins every shove, which is what a giant should feel like. An infinite weight would need a special case in `SeparateCircles`, and nothing in the fight needs it.
- **Slower than a hairball when it walks.** The walk is only how the boss gets close between attacks. If the walk alone could catch the player, the attacks would not matter.
- **The camera leans toward the boss, with a cap.** In a fight with one enemy, losing sight of it is the worst thing the camera can do. Leaning 40% of the way keeps both on screen at the start and in most of the fight. The cap of 200 units keeps the player at least 160 units from the edge of the screen when the boss is across the arena.
  - Rejected: zooming out to fit both. It changes the scale of everything, so the dash and the guns would look shorter or longer depending on where the boss is.
  - Rejected: following the player only. With the boss off screen, the player cannot read its attacks.
- **The boss's hit flash is only part of the way to white.** The SMG would keep a full white flash on for most of the fight. At 45% the hit still shows, and the boss keeps its colour.
- **The health bar is at the bottom, wide and thin.** It reads as the one fight that matters and stays away from the player's own bars in the top left corner.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Health | `Boss::health` (`src/core/Boss.h`) | 1200: 120 pistol or 200 SMG bullets |
| Contact damage | `Boss::contactDamage` | 20 |
| Walking speed | `Boss::stats.maxSpeed` | 120 units/s |
| Acceleration and deceleration | `Boss::stats` | 400 and 1500 units/s^2 |
| Body radius | `Boss::radius` | 60 units |
| Weight | `Boss::weight` | 40 (player 4, hairball 1) |
| Arena | `kBossArena` (`src/core/World.h`) | 1640x1040 |
| Starting distance | `kStartingDistance` (`src/core/World.h`) | 400 units |
| Camera lean and its cap | `kBossFraming`, `kMaxBossFramingOffset` (`src/core/World.cpp`) | 40%, 200 units |
| Look | `kBossStyle`, `kBossBar` and the boss bar constants (`src/app/Game.cpp`) | |

In a Debug build, `F1` shows the boss's health next to the shots fired.

## Tests

- `tests/core/BossTests.cpp`:
  - the boss walks toward the player, slower than a hairball;
  - it stops when it touches the player, and pushes the player rather than being pushed;
  - a touch takes its contact damage, and when the boss and a hairball touch the player in the same step, the boss's hit is the one that lands;
  - a bullet takes health from the boss and flashes it, but does not push it;
  - a hairball in front of the boss takes the bullet meant for the boss;
  - a dash goes through the boss without a scratch;
  - a dead boss does not chase, block, take bullets or hurt;
  - a new fight starts with the camera settled between the player and the boss, with both in view;
  - the camera keeps a far boss in mind without pushing the player off screen;
  - once the boss is dead, the camera goes back to the player alone.
- `tests/core/WorldTests.cpp`: a new world has the boss in the middle and the player south of it, facing it; nobody leaves the arena, the boss included; and two worlds with the boss, hairballs and the same inputs end in exactly the same state.
- `tests/core/SessionTests.cpp`: standing still in front of the boss ends in a game over, between 5 and 8 seconds in (6.1 in practice: about 2.9 s for the boss to arrive, then five touches 0.8 s apart).

The drawing is not unit tested, because it needs a window. I checked it in a Release build with captures that send no input: the boss and the player both on screen at the start, the bar under the boss's name, the boss against the player with the health bar at 20, and the game over screen reading "You lasted 6.1 seconds. The Matriarch had 1200 of 1200 health left." The hit flash, the bar going down and the grey dead boss need someone shooting, so I check them by playing.

## Limitations

- **It has no attacks yet.** It only walks at the player. The charge and the ring of hair come next, then the leap, the shield and the second phase.
- **Killing it does not win the fight yet.** The boss turns grey and stops, and the game goes on. The victory screen comes with the next commit.
- **The bar has no animation.** It jumps to the new value. A trailing chunk that drains behind it is part of the game feel pass.
