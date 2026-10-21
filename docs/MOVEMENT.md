# Movement

## What it is

The rules for how anything in the arena moves: how fast it goes, how quickly it gets to that speed and how quickly it stops. The player uses it first. Enemies will use the same code from day 3.

It lives in `horde_core`, so it has no idea where the input comes from or how the result is drawn:

- `src/core/Vec2.h`: a small 2D vector type with the few operations movement needs.
- `src/core/Movement.h`, `src/core/Movement.cpp`: the `Motion` component, the `MovementStats` tuning values and the `StepMovement` function.

## How it works

Every moving entity has a `Motion` component: `position`, `previousPosition` and `velocity`. Once per fixed step (1/60 s, see [Architecture](ARCHITECTURE.md)), `StepMovement` takes an input vector and does three things:

1. **Clamp the input to length 1.** The input says where the mover wants to go, and its length is how much of the top speed it wants. A keyboard diagonal is `(1, 1)`, which is about 1.41 long, so without the clamp the player would move 41% faster diagonally. An analog stick pushed halfway gives about 0.5, and keeps that half speed.
2. **Move the velocity toward the target velocity** (`input * maxSpeed`) by at most `rate * dt`. The rate is `acceleration` while there is input and `deceleration` when there is none. `MoveTowards` never goes past the target, so a mover that stops does not slide backwards.
3. **Integrate the position** with the new velocity, after saving the old one in `previousPosition`.

The renderer will draw at `Lerp(previousPosition, position, alpha)`, where `alpha` comes from `FixedTimestep::Alpha`. That keeps movement smooth on monitors faster than 60 Hz.

## Design decisions

- **Acceleration toward a target velocity, not instant velocity.** With instant velocity the player starts and stops in one frame, which is precise but looks weightless. A ramp of 0.1 s still feels instant in the hands and reads as weight on screen.
  - Rejected: physics with forces and friction (velocity decays by a factor each step). It never quite reaches zero or top speed, and the numbers are harder to tune by feel.
- **A separate deceleration rate.** Stopping a bit faster than starting makes dodging feel sharp. It is one extra number, and it can be set equal to `acceleration` if I do not want it.
- **Semi-implicit Euler** (update velocity, then move with the new velocity). With a fixed step and no springs it is stable and exact enough, and it is the simplest option.
- **My own `Vec2` instead of raylib's `Vector2`.** `horde_core` must not depend on raylib, so the tests build and run without it. The app converts to `Vector2` where it draws.
  - Rejected: a math library such as glm. It is far bigger than the five functions I need.
- **A free function over a `Motion` struct, instead of a `Player` class with an `Update` method.** Data and behaviour stay apart, so the same function moves the player now and enemies later. This is the "small components and systems" shape described in the [roadmap](ROADMAP.md#architecture-goals).

## How to tune it

The numbers are the defaults in `MovementStats` (`src/core/Movement.h`):

| Field | Default | Effect |
|---|---|---|
| `maxSpeed` | 300 units/s | Top speed. One world unit is one pixel at camera zoom 1. |
| `acceleration` | 3000 units/s^2 | Time to top speed is `maxSpeed / acceleration` (0.1 s). |
| `deceleration` | 4000 units/s^2 | Time to stop from top speed is `maxSpeed / deceleration` (0.075 s). |

A very high value (for example 100000) gives instant start or stop. These values move to JSON on day 6.

## Tests

- `tests/core/Vec2Tests.cpp`: arithmetic, length, `ClampLength`, `MoveTowards` (no overshoot, zero distance) and `Lerp`.
- `tests/core/MovementTests.cpp`: a mover at rest stays still, acceleration reaches top speed and stays there, a diagonal is not faster than a straight line, half stick input gives half speed, releasing stops without sliding back, reversing direction passes through zero, and the step saves the previous position.

The tests use `dt = 0.125` and round numbers, so the expected values are exact.

## Limitations

- Reversing direction uses `acceleration`, not `deceleration`, because there is input. A sharp turn is therefore a little slower than a full stop. If that feels sluggish in play, the fix is to use `deceleration` while the input points against the velocity.
- Nothing stops a mover yet: arena bounds and collisions come in later steps.
- A new entity should set `previousPosition` equal to `position` when it spawns, or its first frame is drawn from the origin.
