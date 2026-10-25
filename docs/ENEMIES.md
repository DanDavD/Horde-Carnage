# Enemies

## What it is

The small enemies of the arena. There is one kind, the hairball: a slow lump of hair that walks straight at the player and stops when it touches them.

The fight no longer starts with hairballs. They are the boss's children, and the boss summons them in its second phase, out of marks on the floor around the player (see [The boss](BOSS.md)). Their code and tests stay, and the boss reuses all of it: the same components, the same chase input and the same movement.

The rules live in `horde_core`:

- `src/core/Enemy.h`: the `Enemy` struct, with the same components as the player (`Motion`, `MovementStats`, a body radius) and the hairball's numbers.
- `src/core/Chase.h`, `src/core/Chase.cpp`: `ChaseInput`, which turns "walk at the player" into a movement input.
- `src/core/World.h`, `src/core/World.cpp`: the hairballs live in the world's enemy pool, and `StepWorld` moves them every fixed step (`StepEnemies`). See [Architecture](ARCHITECTURE.md#the-world).

Hairballs can be shot and killed; that part is in [Bullets and hits](BULLETS.md).

The game executable only draws them:

- `src/app/Game.cpp`: `DrawHairball` draws a round body with tufts of hair and two eyes that look at the player.

## How it works

Nothing in the game spawns hairballs at the moment. The tests put them into the world's pool directly, one at a time or six on a ring around the player.

Once per fixed step, after the player has moved, `StepWorld` does this for every hairball:

1. **Decide where to go** with `ChaseInput(position, playerPosition, stopDistance)`. The stop distance is the hairball's radius plus the player's, the distance at which the two bodies touch, plus `kContactSlack` (1 unit, see [Collisions](COLLISIONS.md)). Farther than that, the input is the direction to the player at length 1. Closer, the input is zero.
2. **Move** with `StepMovement`, the same function the player uses, with the hairball's `MovementStats`. It accelerates toward the player, and brakes when the input drops to zero.
3. **Get pushed and held in.** Once every body has moved, collisions push overlapping bodies apart (hairballs against each other, the player and the boss), and `ConfineToArena` keeps everyone inside the walls. See [Collisions](COLLISIONS.md).

So a hairball rolls toward the player, brakes when it touches them, and starts again as soon as the player steps away.

## Design decisions

- **Enemies reuse the player's components and systems.** A hairball is a `Motion`, a `MovementStats` and a radius, moved by `StepMovement` and held in by `ConfineToArena`. Only the numbers differ. This is why the day 2 movement code is free functions over plain structs, and not a `Player` class.
  - Rejected: an `Enemy` base class with a virtual `Update`. The hairball and the boss differ in how they choose what to do, not in how they move, so the difference belongs in the input (and the boss's state machine), not in a class hierarchy.
- **Chasing is a straight line to the player.** The arena is an open rectangle, so there is nothing to walk around.
  - Rejected: pathfinding (A*). It solves a problem this arena does not have.
- **The hairball stops at contact.** Without a stop distance it aims at the centre of the player, overshoots it, turns back, and shakes on top of the player.
  - Rejected: "arrive" steering, which slows down gradually inside a radius. It stops more smoothly, but adds a tuning value, and a hairball is meant to be dumb. A high deceleration (2000) already limits the slide past contact to `maxSpeed^2 / (2 * deceleration)`, about 6 units.
- **Hairballs are slower than the player** (160 against 300). The player can always run away. A crowd is dangerous because of its numbers, not because one hairball is fast.
- **Hairballs chase the player's position after the player's move in the same step.** If they read it before, they would always aim one step behind.
- **Enemies live in an `ObjectPool` of 64 slots inside the `World`.** Killing and spawning hairballs reuses slots instead of allocating memory. See [Object pool](OBJECT_POOL.md).
- **The eyes look at the player.** A round body has no front, so the eyes show what the hairball is after, even when it stands still.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Top speed | `Enemy::stats.maxSpeed` (`src/core/Enemy.h`) | 160 units/s |
| Acceleration | `Enemy::stats.acceleration` | 600 units/s^2 (0.27 s to top speed) |
| Deceleration | `Enemy::stats.deceleration` | 2000 units/s^2 (stops in about 6 units) |
| Body radius | `Enemy::radius` | 18 units |

In a Debug build, `F1` shows how many enemies the world has.

## Tests

- `tests/core/ChaseTests.cpp`: a far chaser walks straight at its target at length 1, just outside the stop distance it still walks, at contact it stops pushing, and exactly on top of its target it gets zero input instead of a broken direction.
- `tests/core/WorldTests.cpp`: a step moves the player by the movement rules and then moves the camera; six hairballs on a ring get closer to the player; after 10 seconds every hairball has stopped at contact without sinking into the player; nobody leaves the arena when the player runs into a corner; and two worlds given the same inputs end in exactly the same state.

`DrawHairball` is not unit tested, because it needs a window. I check it by playing.

## Limitations

- **Hairballs all aim at the same point, the player.** Collisions keep them from stacking, but they do not plan around each other: the ones behind keep walking into the ones in front. See the limitations in [Collisions](COLLISIONS.md#limitations).
- **Touching the player is the only way a hairball hurts.** It takes 10 hit points per touch, at most one hit every 0.8 s. See [Health](HEALTH.md).
- **Only the boss spawns them.** The Summon, in its second phase, brings four at a time (six in its frenzy), eight alive at most. Those hairballs also spit a strand at the player every 3 s (see [The boss](BOSS.md)); a hairball made any other way does not.
- **When the player walks slowly away, a hairball at contact switches between walking and stopping from one step to the next.** I have not checked how that looks in motion yet, only in still screenshots.
