# Health and contact damage

## What it is

How the player gets hurt. The player has 100 hit points. A touch of the boss takes 20 (30 while it charges), a strand of its Hair Ring takes 15, and a hairball that touches the player takes 10. After each hit the player is invulnerable for 0.8 seconds, and blinks to show it. At 0 the run is over (see [Game flow](GAME_FLOW.md)).

The rules live in `horde_core`:

- `src/core/Health.h`, `src/core/Health.cpp`: the `Health` component, `ApplyDamage`, `TickHealth`, `IsDead` and `IsInvulnerable`.
- `src/core/Collision.h`, `src/core/Collision.cpp`: `AreTouching`, the contact test.
- `src/core/World.cpp`: `StepWorld` counts the invulnerability window down and applies contact damage and the damage of the boss's strands.
- `src/core/Player.h`, `src/core/Boss.h` and `src/core/Enemy.h`: the player has a `Health`, the boss and the hairball have a `contactDamage`, and the boss's attacks have their own damage (`ChargeStats`, `HairRingStats`).

The game executable shows it:

- `src/app/Game.cpp`: `DrawHealthBar` draws the bar in the top left corner, and `DrawPlayer` makes the player blink while invulnerable and turns them grey when dead.

## How it works

Every fixed step, `StepWorld`:

1. **Counts the invulnerability window down** with `TickHealth`, at the start of the step.
2. Moves everyone and resolves collisions (see [Collisions](COLLISIONS.md)).
3. **Applies contact damage:** for the living boss first and then for every hairball, if `AreTouching` says it touches the player (the sum of the radii plus `kContactSlack`), it hurts the player through `HurtPlayer`, the single door for damage to the player. `HurtPlayer` ignores the damage while a dash makes the player invulnerable (see [Player abilities](ABILITIES.md)), and otherwise calls `ApplyDamage` with the body's `contactDamage`, or with the Charge's damage while the boss runs. `HurtPlayer` tells the caller whether the hit landed, so the Charge's knockback is applied only with a hit that counts. A hit that lands also interrupts a flask in progress. Checking the boss first means that when the boss and a hairball arrive in the same step, the boss's heavier hit is the one that lands.
4. **Applies the strands' damage:** `StepHostileProjectiles` moves the boss's strands, and each one that touches a player who is not dashing goes through `HurtPlayer` with 15 and is spent (see [The boss](BOSS.md)). It runs after contact damage, so a touch of the boss (20) wins over a strand (15) that arrives in the same step.

`ApplyDamage` does nothing if the body is dead or still inside its invulnerability window. Otherwise it takes the damage, never going below 0, and opens a new window of `invulnerabilityAfterHit` seconds.

On screen:

- **The health bar** shows `current / max`. Its fill turns pale while the player is invulnerable, so a hit reads on the bar as well as on the body.
- **The player blinks** while invulnerable: faded for 0.1 s, solid for 0.1 s. The blink is computed from the time left in the window, so it needs no extra state.
- **A dead player is grey.**

## Design decisions

- **An invulnerability window after each hit.** A hairball resting against the player touches it on every fixed step, 60 times a second. Without the window, one hairball would take 10 points per step and empty the bar in a sixth of a second, before the player could react. With it, damage arrives at a readable rhythm, and the blink tells the player when the next touch can hurt.
  - Rejected: damage over time (a few points per second while touching). It is harder to read, because the bar drains smoothly and no single moment says "you got hit".
  - Rejected: a cooldown per enemy instead of per player. Six hairballs around the player would each land their own hit, 60 points at once, and being surrounded would kill in under a second. One window per player means a crowd hurts like one hairball, so the player has time to fight their way out once combat exists.
- **Contact damage is checked after collisions.** Then "touching" means the bodies really ended the step touching, not that one passed near the other halfway through.
- **The same contact test for chasing and for damage.** `ChaseInput` stops a hairball at the sum of the radii plus `kContactSlack`, and `AreTouching` uses the same distance. A hairball that has stopped against the player always counts as touching it.
- **Integer hit points.** Every number the player sees is a whole number, and the tests compare health exactly.
  - Rejected: floats. Nothing needs fractions yet, and "73.99999" on a health bar is a bug report.
- **No knockback on the player yet.** It would help the player get out of a crowd, but pushing the player around is the kind of feel decision that belongs with combat on day 4, where enemies get knockback too.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Player hit points | `Health::current` and `Health::max` (`src/core/Health.h`) | 100 |
| Invulnerability after a hit | `Health::invulnerabilityAfterHit` | 0.8 s |
| Boss contact damage | `Boss::contactDamage` (`src/core/Boss.h`) | 20 |
| Boss damage while it charges | `ChargeStats::damage` (`src/core/Boss.h`) | 30 |
| Damage of one strand of the Hair Ring | `HairRingStats::damage` (`src/core/Boss.h`) | 15 |
| Hairball contact damage | `Enemy::contactDamage` (`src/core/Enemy.h`) | 10 |
| Blink period and fade | `kBlinkPeriod`, `kBlinkFade` (`src/app/Game.cpp`) | 0.1 s, 35% |

With the defaults the player survives 4 touches of the boss and dies on the 5th, 6 strands and dies on the 7th, or survives 9 hairball touches and dies on the 10th. Standing still from the start, the boss stalks for 2 seconds and charges: the run hits for 30 at about 2.9 s and ends stunned against the wall next to the player. After the stun it lands a touch every 0.8 s, so a player who does nothing dies after about 7.1 seconds, just before the first Hair Ring. In a Release build the "YOU DIED" screen read "You lasted 0:07" for that case.

In a Debug build, `F1` shows the health and the time left in the window.

## Tests

- `tests/core/HealthTests.cpp`: a hit takes health and opens the window; a second hit inside the window does nothing; once the window has passed the next hit lands; the window never goes below 0; health stops at 0 and the body is dead; and a dead body takes no more hits.
- `tests/core/CollisionTests.cpp`: bodies count as touching up to the contact slack, and not past it.
- `tests/core/WorldTests.cpp`: nothing hurts the player in the first second of the fight; a touching hairball hurts once, not again 0.7 s later, and again 0.9 s later; and when the whole crowd arrives at once the first hit takes exactly one hairball's damage, not six.
- `tests/core/BossTests.cpp`: a touch of the boss takes its contact damage, and when the boss and a hairball touch the player in the same step, the boss's hit is the one that lands; a charge that catches the player takes the charge's damage and throws them out of its path; a dash through the charge takes no damage; the strands of a Hair Ring that reach the player take one strand's damage, and are spent even inside the window; and a dash through the ring takes no damage.

`DrawHealthBar`
 and the blink are not unit tested, because they need a window. I checked them in a Release build.

## Limitations

- **The only healing is the three flasks per attempt** (see [Player abilities](ABILITIES.md)). `Heal` restores health up to the maximum and never revives.
- **Hairballs and the boss use the same `Health` component with no invulnerability window**, so every bullet counts. See [Bullets and hits](BULLETS.md).
- **The bar has no animation.** It jumps to the new value. A trailing "damage" bar that drains behind it is part of the game feel pass on day 10.
