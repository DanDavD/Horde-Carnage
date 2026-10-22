# Enemies

## What it is

The enemies of the arena. For now there is one kind, the hairball: a slow lump of hair that walks straight at the player and stops when it touches them. It is the basic enemy of the horde, the one every wave will be full of.

The rules live in `horde_core`:

- `src/core/Enemy.h`: the `Enemy` struct, with the same components as the player (`Motion`, `MovementStats`, a body radius) and the hairball's numbers.
- `src/core/Chase.h`, `src/core/Chase.cpp`: `ChaseInput`, which turns "walk at the player" into a movement input.
- `src/core/World.h`, `src/core/World.cpp`: `CreateWorld` places the hairballs, and `StepWorld` moves them every fixed step. See [Architecture](ARCHITECTURE.md#the-world).

The game executable only draws them:

- `src/app/Game.cpp`: `DrawHairball` draws a round body with tufts of hair and two eyes that look at the player.

## How it works

`CreateWorld` puts `kStartingHairballs` (6) hairballs on a ring of radius `kHairballSpawnDistance` (520 units) around the player, evenly spaced. The view is 1280x720, so some start on screen and some just off it, and they come from every side at once.

Once per fixed step, after the player has moved, `StepWorld` does this for every hairball:

1. **Decide where to go** with `ChaseInput(position, playerPosition, stopDistance)`. The stop distance is the hairball's radius plus the player's, the distance at which the two bodies touch. Farther than that, the input is the direction to the player at length 1. Closer, the input is zero.
2. **Move** with `StepMovement`, the same function the player uses, with the hairball's `MovementStats`. It accelerates toward the player, and brakes when the input drops to zero.
3. **Stay inside the arena** with `ConfineToArena`, like the player.

So a hairball rolls toward the player, brakes when it touches them, and starts again as soon as the player steps away.

## Design decisions

- **Enemies reuse the player's components and systems.** A hairball is a `Motion`, a `MovementStats` and a radius, moved by `StepMovement` and held in by `ConfineToArena`. Only the numbers differ. This is why the day 2 movement code is free functions over plain structs, and not a `Player` class.
  - Rejected: an `Enemy` base class with a virtual `Update`. The chaser, charger and spitter of day 7 differ in how they choose their input, not in how they move, so the difference belongs in the input (and later a state machine), not in a class hierarchy.
- **Chasing is a straight line to the player.** The arena is an open rectangle, so there is nothing to walk around.
  - Rejected: pathfinding (A*). It solves a problem this arena does not have.
- **The hairball stops at contact.** Without a stop distance it aims at the centre of the player, overshoots it, turns back, and shakes on top of the player.
  - Rejected: "arrive" steering, which slows down gradually inside a radius. It stops more smoothly, but adds a tuning value, and a hairball is meant to be dumb. A high deceleration (2000) already limits the slide past contact to `maxSpeed^2 / (2 * deceleration)`, about 6 units.
- **Hairballs are slower than the player** (160 against 300). The player can always run away. A horde is dangerous because of its numbers, not because one enemy is fast.
- **Hairballs chase the player's position after the player's move in the same step.** If they read it before, they would always aim one step behind.
- **The enemy list is sized once, in `CreateWorld`.** `StepWorld` never adds or removes enemies, so a running game does not allocate memory. When enemies start dying and spawning (days 4 and 5), the list becomes an `ObjectPool`.
- **The eyes look at the player.** A round body has no front, so the eyes show what the hairball is after, even when it stands still.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Top speed | `Enemy::stats.maxSpeed` (`src/core/Enemy.h`) | 160 units/s |
| Acceleration | `Enemy::stats.acceleration` | 600 units/s^2 (0.27 s to top speed) |
| Deceleration | `Enemy::stats.deceleration` | 2000 units/s^2 (stops in about 6 units) |
| Body radius | `Enemy::radius` | 18 units |
| Number at the start | `kStartingHairballs` (`src/core/World.h`) | 6 |
| Distance from the player at the start | `kHairballSpawnDistance` (`src/core/World.h`) | 520 units |

In a Debug build, `F1` shows how many enemies the world has.

## Tests

- `tests/core/ChaseTests.cpp`: a far chaser walks straight at its target at length 1, just outside the stop distance it still walks, at contact it stops pushing, and exactly on top of its target it gets zero input instead of a broken direction.
- `tests/core/WorldTests.cpp`: a new world has the player in the middle and the hairballs on the ring, standing still and inside the arena; the camera starts on the player; a step moves the player by the movement rules and then moves the camera; hairballs get closer to the player; after 10 seconds every hairball has stopped at contact, no more than 10 units inside it; nobody leaves the arena when the player runs into a corner; and two worlds given the same inputs end in exactly the same state.

`DrawHairball` is not unit tested, because it needs a window. I check it by playing.

## Limitations

- **Hairballs pass through each other.** Coming from every side they overlap a little around the player, and when the player runs away they all aim at the same point behind the player and pile up into one. Collisions between bodies come in the next step of day 3.
- **Touching the player does nothing yet.** Contact damage and the player's health come later on day 3.
- **There are always exactly 6.** They cannot be killed (day 4), and there are no waves (day 5).
- **When the player walks slowly away, a hairball at contact switches between walking and stopping from one step to the next.** I have not checked how that looks in motion yet, only in still screenshots.
