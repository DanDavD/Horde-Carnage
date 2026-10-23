# Bullets and hits

## What it is

What happens after the trigger: a bullet leaves the muzzle, flies in a straight line, and either hits a hairball, reaches the end of its range, or leaves the arena. A hit takes health, flashes the hairball white, stops it chasing for a moment and pushes it back along the bullet's flight. A hairball at 0 health dies and is counted as a kill. Once the arena is empty, a new ring of hairballs arrives two seconds later.

The rules live in `horde_core`:

- `src/core/Projectile.h`, `src/core/Projectile.cpp`: the `Projectile` component, `MakeBullet` and `StepProjectile`.
- `src/core/Weapon.h`: each gun's damage, bullet speed, range, knockback and barrel length, and `MuzzlePoint`.
- `src/core/Collision.h`, `src/core/Collision.cpp`: `SweepCircle`, the hit test.
- `src/core/Enemy.h`: the hairball's `Health` (30 hit points), and its hit flash and stun timers.
- `src/core/World.cpp`: firing (`HandleWeapons`), flight and hits (`StepBullets`, `FirstEnemyHit`, `HitEnemy`), deaths (`RemoveDeadEnemies`) and the new ring (`RefillEmptyArena`).

The game executable draws bullets as short yellow streaks, flashes hit hairballs white, and shows the kill count in the top right corner and on the game over screen.

## How it works

**Firing.** When `PullTrigger` fires (see [Aiming and weapons](WEAPONS.md)), `HandleWeapons` spawns a bullet into the bullet pool (see [Object pool](OBJECT_POOL.md)) at `MuzzlePoint`: the player's centre, plus the facing direction times the body radius plus the barrel length. Its velocity is the facing direction times the gun's bullet speed, and its lifetime is the gun's range divided by that speed.

**Flight and hits.** Every fixed step, after bodies have moved and collided, `StepBullets` goes through the pool once. For each bullet:

1. `StepProjectile` moves it along its velocity and counts its lifetime down.
2. `FirstEnemyHit` sweeps the bullet's circle from where it was to where it is now against every living hairball, with `SweepCircle`, and keeps the one it touches first along the way.
3. If it hit, `HitEnemy` applies the damage, starts a 0.1 s white flash and a 0.15 s stun, and adds the bullet's knockback to the hairball's velocity, along the bullet's direction. The bullet is removed.
4. Otherwise the bullet is removed if its lifetime is over or it is outside the arena.

**Stun and knockback.** The knockback slides the hairball away, braked at its deceleration, and the collisions let it shove the hairballs behind it. While stunned it gets no chase input, so once it stops it stays where it landed for the rest of the stun instead of turning straight back toward the player.

**Deaths and the next ring.** After the bullets, `RemoveDeadEnemies` removes every hairball at 0 health from the pool and adds them to `kills`. This happens before contact damage, so a hairball killed in a step cannot hurt the player in the same step. When the pool is empty, `RefillEmptyArena` counts 2 seconds and spawns a new ring around the player with `SpawnHairballRing`, the same function `CreateWorld` uses.

## Design decisions

- **Bullets are objects that fly, not instant rays.** A visible bullet with travel time is what SAS 4 and Dead Ops Arcade feel like: the player leads moving targets and sees the stream.
  - Rejected: hitscan (an instant ray from the muzzle). It is simpler and never misses a fast target, but there is nothing to see and nothing to dodge, and the spitter of day 7 needs travelling projectiles anyway.
- **A swept test instead of checking where the bullet ends the step.** At 950 units a second a bullet moves about 16 units a step. A hairball is 36 wide, so a point test would work today, but a faster gun would skip over enemies, and a point test cannot tell which of two enemies in a line came first. `SweepCircle` solves both with one quadratic.
- **The nearest hit wins, and a bullet hits one enemy.** A bullet that stops on the first body reads like a bullet. Piercing is a good upgrade for day 9.
- **Hairballs have no invulnerability window.** The player's 0.8 s window exists so a crowd cannot drain the bar in a moment. For enemies it would cut the SMG's 10 bullets a second down to one hit every 0.8 s, so it is 0.
- **A short stun with the knockback.** The slide is the same either way, because chasing back toward the player counts as braking, at the same rate as having no input. The difference is what comes after: without the stun, the hairball turns around and walks back the moment it stops, and the hit reads as a twitch. With it, the hairball holds still a moment where it landed, which reads as a stagger.
- **Knockback is a velocity, not a teleport.** It goes through the same movement and collision code as everything else, so a knocked-back hairball slows down naturally, pushes the ones behind it, and stops at the wall.
- **The muzzle position is one function.** `MuzzlePoint` uses the barrel length from `WeaponStats`, and both the simulation (where bullets start) and the renderer (where the barrel ends) call it, so they cannot drift apart.
- **A dead hairball stays in the pool until all bullets of the step have flown, but cannot be hit.** Removing it in the middle of `StepBullets` would move other enemies around while bullets are being tested against them. Skipping dead ones in `FirstEnemyHit` keeps a second bullet from being wasted on a corpse.
- **A cleared arena refills after 2 seconds.** It is a stand-in so the game keeps going until the wave director of day 5 replaces it.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Damage per bullet | `WeaponStats::damage` (`src/core/Weapon.h`) | Pistol 10, SMG 6 |
| Bullet speed | `WeaponStats::bulletSpeed` | Pistol 950, SMG 850 units/s |
| Range | `WeaponStats::range` | Pistol 750, SMG 600 units |
| Knockback | `WeaponStats::knockback` | Pistol 300, SMG 120 units/s |
| Barrel length (where bullets start) | `WeaponStats::barrelLength` | Pistol 14, SMG 22 units |
| Bullet radius | `Projectile::radius` (`src/core/Projectile.h`) | 4 units |
| Hairball health | `Enemy::health` (`src/core/Enemy.h`) | 30: 3 pistol bullets, 5 SMG bullets |
| Hit flash and stun | `kHitFlashSeconds`, `kHitStunSeconds` (`src/core/World.cpp`) | 0.1 s, 0.15 s |
| Delay before a new ring | `kRespawnDelaySeconds` (`src/core/World.h`) | 2 s |

In a Debug build, `F1` shows the number of live enemies and bullets, the shots fired and the kills.

## Tests

- `tests/core/ProjectileTests.cpp`: a bullet leaves the muzzle with the gun's speed, damage and knockback; a step moves it in a straight line and remembers where it was; and it flies its range, within one step, and then stops.
- `tests/core/CollisionTests.cpp`: a sweep touches a circle at the right fraction of the move; a sweep that passes beside a circle or stops short misses; a sweep that starts touching hits at once; a fast circle cannot skip over a thin one between two steps; and a circle that does not move only hits what it already touches.
- `tests/core/ArenaTests.cpp`: an arena contains the points inside it and on its edge, and nothing outside.
- `tests/core/WeaponTests.cpp`: the muzzle is at the end of the barrel; the pistol hits harder per bullet and the SMG fires faster.
- `tests/core/WorldTests.cpp`: a shot leaves the muzzle and flies where the player aims; a bullet that hits nothing is gone after its range; a bullet that reaches a wall is gone at once; three pistol bullets kill a hairball and the kill is counted; a hit flashes, stuns and knocks a hairball back along the bullet; a bullet hits only the nearest of two hairballs in a line; a hairball killed in a step is gone before it can touch the player; and a cleared arena gets a new ring after the delay. The determinism test now fires too, and compares kills and bullets.

The drawing is not unit tested, because it needs a window. In a Debug build, holding the SMG toward the hairball on the right killed it, and the overlay showed `enemies 5  bullets 5` and `shots 13  kills 1`.

## Limitations

- **The SMG has no spread.** Every bullet goes exactly where the player aims. Spread needs a seeded random generator in the world, so that the simulation stays deterministic; it belongs with the gun upgrades of day 9.
- **Bullets ignore the player and each other.** There is no friendly fire, and bullets do not collide in the air.
- **The new ring is centred on the player.** Near a wall part of it spawns outside the arena, and the walls push those hairballs back in, so they can arrive clumped. The wave director of day 5 replaces the ring.
- **No sound, impact particles or screen shake yet.** Those are day 12.
