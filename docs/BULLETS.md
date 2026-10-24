# Bullets and hits

## What it is

What happens after the trigger: a bullet leaves the muzzle, flies in a straight line, and either hits a body (a hairball or the boss), reaches the end of its range, or leaves the arena. A hit on a hairball takes health, flashes it white, stops it chasing for a moment and pushes it back along the bullet's flight. A hairball at 0 health dies and is removed. A hit on the boss only takes health and flashes it (see [The boss](BOSS.md)).

The rules live in `horde_core`:

- `src/core/Projectile.h`, `src/core/Projectile.cpp`: the `Projectile` component, `LaunchProjectile`, `MakeBullet` and `StepProjectile`. The boss's strands are the same component, in a pool of their own (see [The boss](BOSS.md)).
- `src/core/Weapon.h`: each gun's damage, bullet speed, range, knockback and barrel length, and `MuzzlePoint`.
- `src/core/Collision.h`, `src/core/Collision.cpp`: `SweepCircle`, the hit test.
- `src/core/Enemy.h`: the hairball's `Health` (30 hit points), and its hit flash and stun timers.
- `src/core/World.cpp`: firing (`HandleWeapons`), flight and hits (`StepBullets`, `FirstHit`, `HitEnemy`, `HitBoss`) and deaths (`RemoveDeadEnemies`).

The game executable draws bullets as short yellow streaks and flashes the bodies they hit.

## How it works

**Firing.** When `PullTrigger` fires (see [Aiming and weapons](WEAPONS.md)), `HandleWeapons` spawns a bullet into the bullet pool (see [Object pool](OBJECT_POOL.md)) at `MuzzlePoint`: the player's centre, plus the facing direction times the body radius plus the barrel length. `MakeBullet` builds it with `LaunchProjectile`: its velocity is the facing direction times the gun's bullet speed, and its lifetime is the gun's range divided by that speed. Then it takes the gun's damage and knockback.

**Flight and hits.** Every fixed step, after bodies have moved and collided, `StepBullets` goes through the pool once. For each bullet:

1. `StepProjectile` moves it along its velocity and counts its lifetime down.
2. `FirstHit` sweeps the bullet's circle from where it was to where it is now against every living hairball and the living boss, unless the boss is in the air during its Leap, with `SweepCircle`, and keeps the body it touches first along the way.
3. If it hit a hairball, `HitEnemy` applies the damage, starts a 0.1 s white flash and a 0.15 s stun, and adds the bullet's knockback to the hairball's velocity, along the bullet's direction. If it hit the boss, `HitBoss` applies the damage and starts the flash. Either way the bullet is removed.
4. Otherwise the bullet is removed if its lifetime is over or it is outside the arena.

**Stun and knockback.** The knockback slides the hairball away, braked at its deceleration, and the collisions let it shove the hairballs behind it. While stunned it gets no chase input, so once it stops it stays where it landed for the rest of the stun instead of turning straight back toward the player.

**Deaths.** After the bullets, `RemoveDeadEnemies` removes every hairball at 0 health from the pool. This happens before contact damage, so a hairball killed in a step cannot hurt the player in the same step. A dead boss stays in the world but takes no part in it (see [The boss](BOSS.md)).

## Design decisions

- **Bullets are objects that fly, not instant rays.** A visible bullet with travel time is what SAS 4 and Dead Ops Arcade feel like: the player leads moving targets and sees the stream.
  - Rejected: hitscan (an instant ray from the muzzle). It is simpler and never misses a fast target, but there is nothing to see and nothing to dodge, and the boss's ranged attacks need travelling projectiles anyway.
- **A swept test instead of checking where the bullet ends the step.** At 950 units a second a bullet moves about 16 units a step. A hairball is 36 wide, so a point test would work today, but a faster gun would skip over enemies, and a point test cannot tell which of two enemies in a line came first. `SweepCircle` solves both with one quadratic.
- **The nearest hit wins, and a bullet hits one body.** A bullet that stops on the first body reads like a bullet, and a hairball in front of the boss shields it.
- **Hairballs have no invulnerability window.** The player's 0.8 s window exists so a crowd cannot drain the bar in a moment. For enemies it would cut the SMG's 10 bullets a second down to one hit every 0.8 s, so it is 0.
- **A short stun with the knockback.** The slide is the same either way, because chasing back toward the player counts as braking, at the same rate as having no input. The difference is what comes after: without the stun, the hairball turns around and walks back the moment it stops, and the hit reads as a twitch. With it, the hairball holds still a moment where it landed, which reads as a stagger.
- **Knockback is a velocity, not a teleport.** It goes through the same movement and collision code as everything else, so a knocked-back hairball slows down naturally, pushes the ones behind it, and stops at the wall.
- **The muzzle position is one function.** `MuzzlePoint` uses the barrel length from `WeaponStats`, and both the simulation (where bullets start) and the renderer (where the barrel ends) call it, so they cannot drift apart.
- **A dead hairball stays in the pool until all bullets of the step have flown, but cannot be hit.** Removing it in the middle of `StepBullets` would move other enemies around while bullets are being tested against them. Skipping dead ones in `FirstHit` keeps a second bullet from being wasted on a corpse.
- **The boss's projectiles are `Projectile`s too, in another pool.** A strand of the Hair Ring moves exactly like a bullet, so it shares the component, `LaunchProjectile` and `StepProjectile`. The pool decides who it can hit: `StepBullets` only sees the player's bullets, and the boss's strands are handled by `StepHostileProjectiles`, so a bullet can never hit the player and a strand can never hit the boss.

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
| Boss health | `Boss::health` (`src/core/Boss.h`) | 1200: 120 pistol bullets, 200 SMG bullets |

In a Debug build, `F1` shows the number of live enemies and bullets, the shots fired and the boss's health.

## Tests

- `tests/core/ProjectileTests.cpp`: a launched projectile leaves its origin at its speed, with a lifetime of its range; a bullet leaves the muzzle with the gun's speed, damage and knockback; a step moves it in a straight line and remembers where it was; and it flies its range, within one step, and then stops.
- `tests/core/CollisionTests.cpp`: a sweep touches a circle at the right fraction of the move; a sweep that passes beside a circle or stops short misses; a sweep that starts touching hits at once; a fast circle cannot skip over a thin one between two steps; and a circle that does not move only hits what it already touches.
- `tests/core/ArenaTests.cpp`: an arena contains the points inside it and on its edge, and nothing outside.
- `tests/core/WeaponTests.cpp`: the muzzle is at the end of the barrel; the pistol hits harder per bullet and the SMG fires faster.
- `tests/core/WorldTests.cpp`: a shot leaves the muzzle and flies where the player aims; a bullet that hits nothing is gone after its range; a bullet that reaches a wall is gone at once; three pistol bullets kill a hairball and it is removed; a hit flashes, stuns and knocks a hairball back along the bullet; a bullet hits only the nearest of two hairballs in a line; and a hairball killed in a step is gone before it can touch the player. The determinism test fires too, and compares the bullets and the boss's health.
- `tests/core/BossTests.cpp`: a bullet takes health from the boss and flashes it without pushing it; a hairball in front of the boss takes the bullet meant for the boss; a bullet and a strand of the boss's ring fly through each other; and bullets fly under the boss while it is in the air.

The drawing is not unit tested, because it needs a window. In a Debug build, holding the SMG toward a hairball killed it, and the overlay counted the enemies, bullets and shots.

## Limitations

- **The SMG has no spread.** Every bullet goes exactly where the player aims. Spread needs a seeded random generator in the world, so that the simulation stays deterministic. The generator arrives with the boss's attack patterns (day 7).
- **Bullets ignore the player and each other.** There is no friendly fire, and bullets do not collide in the air, with each other or with the boss's strands. Shooting the Hair Ring does nothing.

- **No sound, impact particles or screen shake yet.** Those are day 10.
