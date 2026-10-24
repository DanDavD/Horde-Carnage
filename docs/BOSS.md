# The boss

## What it is

The Matriarch, the one boss the game is about: a giant hairball, mother of the small ones, in the middle of the arena. The whole game is one fight against it, in the spirit of Gun Bros and Dark Souls 1: a boss that is hard on purpose, whose patterns you learn by dying. See the [roadmap](ROADMAP.md#changes-to-the-plan) for why.

This document describes what exists today. The boss stalks the player, shoves them, hurts on touch, soaks up bullets, and has its first attack: the Charge. The Hair Ring comes next, then the Leap, the shield and the second phase.

The rules live in `horde_core`:

- `src/core/Boss.h`, `src/core/Boss.cpp`: the `Boss` struct, with the same body components as the player and the hairballs (`Motion`, `MovementStats`, a radius and a weight) and the boss's numbers; its attack state (`BossAttack`, `AttackStage`, `ChargeStats`); and the systems that run it: `StepBoss`, `IsCharging` and `PlanChargePath`.
- `src/core/World.h`, `src/core/World.cpp`: `kBossArena` and `kStartingDistance`; `CreateWorld` places the boss; `StepWorld` calls `StepBoss`; the collision, bullet and contact damage systems include the boss; and `CameraFocus` keeps it in view.

The game executable draws it:

- `src/app/Game.cpp`: `DrawBoss` draws the same hairy body as a hairball (`DrawHairBody`), three times bigger, darker, with glowing eyes, plus the crouch, the ghosts and the stun stars of the Charge. `DrawChargeTelegraph` draws the Charge's path on the floor. `DrawBossBar` draws its health along the bottom of the screen, under its name.

## How it works

**The start.** `CreateWorld` puts the boss in the middle of the arena (1640x1040) and the player `kStartingDistance` (400 units) south of it, facing it. There are no hairballs.

**Stalking and attacking.** The boss is always doing one of two things:

```
  Stalking  -- 2 s of stalking -->  Attack: Warning --> Active --> Recovery
      ^                                                                 |
      +---------------------------- recovery over ----------------------+
```

- **Stalking** (`Boss::attack` is empty): it walks at the player with the same `ChaseInput` as a hairball, stopping at contact, at 120 units/s at most and slow to get going (400 units/s^2). `secondsUntilAttack` counts down from `attackInterval` (2 s).
- **Attacking** (`Boss::attack` holds a `BossAttack`): every attack runs through the same three stages, `Warning`, `Active` and `Recovery`, and `stageSeconds` is the clock of the current one. When the recovery ends, the boss stalks again for another 2 s.

**The Charge,** stage by stage:

1. **Warning, 0.8 s.** The boss plants itself and crouches. Its path is drawn on the floor: a band as wide as its body, from where it stands to where the run will end. For the first 0.5 s the path follows the player. For the last 0.3 s (`aimLockSeconds`) it is locked, drawn brighter, and the boss trembles. That is the moment to get out of the way.
2. **Active.** The boss runs along the locked direction at 1000 units/s for up to 800 units. Each step `PlanChargePath` works out where the run ends: after the full 800 units, or earlier, where the body meets a wall. While it runs, the boss passes through the player instead of shoving them, still bowls hairballs aside, and a touch takes 30 instead of 20 and throws the player sideways out of the path at 700 units/s.
3. **Recovery.** If the run ended at a wall, the boss is stunned for 1.5 s, with stars circling over its head: the opening to punish it. If it ended in the open, it recovers in 0.5 s.

**Every fixed step,** after the player and the hairballs have moved:

1. **Think and move.** `StepBoss` stalks, or advances the attack's stage.
2. **Collide.** The collision passes treat the boss as one more body (see [Collisions](COLLISIONS.md)). At weight 40 against the player's 4, the player takes 91% of any overlap, so the boss shoves the player and the player cannot shove it. A dashing player passes through it, and a charging boss passes through the player.
3. **Take bullets.** For each bullet, `FirstHit` sweeps it against every hairball and the boss, and the body it touches first takes it. `HitBoss` takes the bullet's damage and starts a 0.1 s flash. No stun and no knockback.
4. **Hurt on touch.** `ApplyContactDamage` checks the boss before the hairballs. A touch takes 20, or 30 plus the knockback while it charges, through `HurtPlayer`, the single door for damage (see [Health](HEALTH.md)). A dash protects from both.

**The camera** follows `CameraFocus`: the player, pulled 40% of the way toward the boss, but never more than 200 units. See [Camera](CAMERA.md).

**Death.** At 0 health the boss is out of the simulation: `StepBoss` drops its attack and brakes it to a stop, it is no longer a body in the collisions, bullets fly through it, it does not hurt, and the camera goes back to the player. It is drawn grey with dark eyes. `IsBossDefeated` turns true, so the session ends the fight in the same step with the "VICTORY" screen (see [Game flow](GAME_FLOW.md)).

## Design decisions

- **A type of its own, not an `Enemy` with bigger numbers.** The boss shares every body system with the hairballs, but its rules differ: bullets neither stagger nor push it, it attacks with a warning, an active part and a recovery, and it will change phase. Putting that into `Enemy` would give every hairball fields it never uses, and a flag checked everywhere.
  - Rejected: a boss flag in `Enemy`. Every system would ask "is this the boss?", and the hairball tests would carry the boss's rules.
  - Rejected: a base class with virtual behaviour. As with the hairballs, the difference is in what the body decides to do, not in how it moves, so it belongs in data and systems.
- **Exactly one boss, as a plain member of `World`.** The game is one fight. A pool or a list for one object would only add loops and empty checks.
- **A dead boss is inert, not removed.** The `World` always has its boss, so no system needs to ask whether there is one. The tests about the player and the hairballs use a world with a dead boss for the same reason: it is the boss "not being there" by the game's own rules.
- **Every attack has the same three stages.** The player learns one rhythm for the whole fight: watch the warning, survive the attack, punish the recovery. In code, the stages are an `enum` and one clock, and each attack only decides what happens inside them. The Hair Ring and the Leap will reuse them as they are.
  - Rejected: a state per attack and stage (`ChargeWarning`, `Charging`, `ChargeRecovery`, `RingWarning`...). Five attacks would make fifteen states that repeat the same timing code, and nothing would force them to keep the same rhythm.
  - Rejected: a class per attack with virtual `Update`. The attacks share all their data except a direction and a distance, so a hierarchy would be more code than behaviour, and it would put the attacks in the heap instead of in the plain `Boss` struct.
- **The aim locks before the run.** If the path followed the player until the last moment, the only answer would be the dash. With 0.3 s of locked path, a quick sidestep also works, and the bright path and the tremble tell the player exactly when to move.
- **The telegraph draws the real path.** `DrawChargeTelegraph` calls the same `PlanChargePath` that the run follows, so the band on the floor ends exactly where the boss will stop, and the bar across its end appears only when the run will end at a wall.
- **A fixed distance, not "until a wall".** At 800 units the run overshoots the player by a lot and usually ends at a wall, but a charge from near one wall into the open middle stops short and recovers fast. So where the player stands decides how big the opening is, and leading the boss into a wall is a skill.
- **The run goes through the player.** At 1000 units/s a shove would carry the player across the arena and pin them against the far wall. Instead the hit lands once (the invulnerability window covers the rest of the overlap) and the knockback throws the player sideways, out of the path.
- **No stagger and no knockback from bullets.** The SMG hits ten times a second. If each bullet staggered the boss, the SMG would lock it in place, and the fight would be about holding the button. The openings to punish it come from its own attacks (the recovery after each one) and will come from its shield.
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
| Stalking time between attacks | `Boss::attackInterval` | 2 s |
| Charge warning, and its locked part | `ChargeStats::warningSeconds`, `aimLockSeconds` | 0.8 s, the last 0.3 s |
| Charge speed and distance | `ChargeStats::speed`, `distance` | 1000 units/s, 800 units |
| Charge damage and knockback | `ChargeStats::damage`, `knockback` | 30, 700 units/s |
| Recovery in the open, stun at a wall | `ChargeStats::recoverySeconds`, `stunSeconds` | 0.5 s, 1.5 s |
| Arena | `kBossArena` (`src/core/World.h`) | 1640x1040 |
| Starting distance | `kStartingDistance` (`src/core/World.h`) | 400 units |
| Camera lean and its cap | `kBossFraming`, `kMaxBossFramingOffset` (`src/core/World.cpp`) | 40%, 200 units |
| Look | `kBossStyle`, `kBossBar`, the boss bar constants and the `kCharge...` and `kStun...` constants (`src/app/Game.cpp`) | |

In a Debug build, `F1` shows the boss's health next to the shots fired and hit, and what the boss is doing: stalking and the time to its next attack, or the attack's stage and its clock. `F4` defeats the boss at once (`DefeatBoss`), to reach the victory screen without shooting.

## Tests

- `tests/core/BossChargeTests.cpp`, on the boss alone, without a world:
  - the boss stalks the player, then starts a charge after its attack interval;
  - during the warning it stands still, and its aim follows the player until it locks;
  - the charge runs in a straight line at its speed;
  - a charge into a wall stops at the wall and stuns the boss for the long recovery, then the boss stalks again with the whole interval ahead;
  - a charge that runs out in the open stops after its distance and recovers quickly;
  - a charge path runs its full distance in the open and stops at the first wall, also on a diagonal;
  - a dead boss drops its attack and stays where it fell.
- `tests/core/BossTests.cpp`, in a world:
  - the boss walks toward the player, slower than a hairball;
  - it stops when it touches the player, and pushes the player rather than being pushed;
  - a touch takes its contact damage, and when the boss and a hairball touch the player in the same step, the boss's hit is the one that lands;
  - a bullet takes health from the boss and flashes it, but does not push it;
  - a hairball in front of the boss takes the bullet meant for the boss;
  - a dash goes through the boss without a scratch;
  - a dead boss does not chase, block, take bullets or hurt;
  - a charge that catches the player takes the charge's damage and throws them out of its path;
  - the charge runs through the player instead of carrying them along, and ends stunned at the wall;
  - a dash through the charge takes no damage;
  - the boss counts as charging only while it runs;
  - a new fight starts with the camera settled between the player and the boss, with both in view;
  - the camera keeps a far boss in mind without pushing the player off screen;
  - once the boss is dead, the camera goes back to the player alone.
- `tests/core/WorldTests.cpp`: a new world has the boss in the middle and the player south of it, facing it; nobody leaves the arena, the boss included; and two worlds with the boss, hairballs and the same inputs end in exactly the same state.
- `tests/core/SessionTests.cpp`: standing still in front of the boss ends in death between 6.5 and 7.5 seconds in (7.1 in practice: the charge hits for 30 at about 2.9 s and ends stunned against the wall next to the player, then four touches 0.8 s apart); and the last bullet into the boss wins the fight in the step it lands.

The drawing is not unit tested, because it needs a window. I checked it in a Release build with captures that send no input, standing still from the start: the path following the player during the warning, the brighter locked path with the boss crouched, the ghosts behind the running boss, the stun stars over the boss against the south wall with the player's health at 70, and the "YOU DIED" screen at 0:07. The hit flash, the bar going down, dodging the charge and punishing the stun need someone playing, so I check them by playing.

## Limitations

- **The Charge is its only attack.** It charges every time. The Hair Ring comes in the next commit; the Leap and the choice between attacks (weighted, seeded, never the same twice in a row) come on day 7.
- **One bar is the whole fight.** Emptying it wins at once. The transformation and the second phase come on day 8.
- **The crouch is a shrink, not a squash.** The body is drawn with circles, so it gets smaller instead of flatter. Squash and stretch come with the art pass.
- **At the south wall the boss is partly hidden by the HUD.** The boss bar and the controls line cover the bottom of the screen, where the camera stops for the south wall. The art pass will rework the HUD.
- **The bar has no animation.** It jumps to the new value. A trailing chunk that drains behind it is part of the game feel pass.
