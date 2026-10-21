# Movement

## What it is

The rules for how anything in the arena moves: how fast it goes, how quickly it gets to that speed, how quickly it stops, and how the arena walls hold it in. The player uses it first. Enemies will use the same code from day 3.

The rules live in `horde_core`, so they have no idea where the input comes from or how the result is drawn:

- `src/core/Vec2.h`: a small 2D vector type with the few operations movement needs.
- `src/core/Movement.h`, `src/core/Movement.cpp`: the `Motion` component, the `MovementStats` tuning values, `MotionAt` to spawn a mover and the `StepMovement` function.
- `src/core/Arena.h`, `src/core/Arena.cpp`: the `Arena` rectangle and `ConfineToArena`, which keeps a mover inside its walls.
- `src/core/AnalogInput.h`: `ApplyRadialDeadzone` for gamepad sticks.
- `src/core/Player.h`: the player's data (`Motion`, `MovementStats` and a body radius).

The game executable connects them to the hardware and the screen:

- `src/app/PlayerInput.h`, `src/app/PlayerInput.cpp`: `ReadMoveInput` reads the keyboard and the first gamepad.
- `src/app/Game.cpp`: steps the player every fixed step and draws the arena and the player.

## How it works

Every moving entity has a `Motion` component: `position`, `previousPosition` and `velocity`. It is created with `MotionAt(position)`, which sets both positions to the same point. Once per fixed step (1/60 s, see [Architecture](ARCHITECTURE.md)), `Game::Update` does this for the player:

1. **Read the input** with `ReadMoveInput`. WASD and the arrow keys give `-1`, `0` or `1` on each axis. The left stick of the first gamepad goes through a radial deadzone of 0.2. The two are added together.
2. **Step the movement** with `StepMovement`:
   1. **Clamp the input to length 1.** The input says where the mover wants to go, and its length is how much of the top speed it wants. A keyboard diagonal is `(1, 1)`, which is about 1.41 long, so without the clamp the player would move 41% faster diagonally. An analog stick pushed halfway gives about 0.5, and keeps that half speed.
   2. **Move the velocity toward the target velocity** (`input * maxSpeed`) by at most `rate * dt`. The rate is `acceleration` while there is input and `deceleration` when there is none. `MoveTowards` never goes past the target, so a mover that stops does not slide backwards.
   3. **Integrate the position** with the new velocity, after saving the old one in `previousPosition`.
3. **Confine it to the arena** with `ConfineToArena`. On each axis, if the body (a circle of `radius`) went through a wall, it is put back against the wall, and the part of the velocity that points into that wall is set to zero. The other axis is not touched, so the player slides along walls.

The renderer draws the player at `Lerp(previousPosition, position, alpha)`, where `alpha` comes from `FixedTimestep::Alpha`. That keeps movement smooth on monitors faster than 60 Hz.

World units are pixels for now, and y grows downward like on screen. The arena is 1200x620 and fits the window, so there is no camera yet.

## Design decisions

- **Acceleration toward a target velocity, not instant velocity.** With instant velocity the player starts and stops in one frame, which is precise but looks weightless. A ramp of 0.1 s still feels instant in the hands and reads as weight on screen.
  - Rejected: physics with forces and friction (velocity decays by a factor each step). It never quite reaches zero or top speed, and the numbers are harder to tune by feel.
- **A separate deceleration rate.** Stopping a bit faster than starting makes dodging feel sharp. It is one extra number, and it can be set equal to `acceleration` if I do not want it.
- **Semi-implicit Euler** (update velocity, then move with the new velocity). With a fixed step and no springs it is stable and exact enough, and it is the simplest option.
- **My own `Vec2` instead of raylib's `Vector2`.** `horde_core` must not depend on raylib, so the tests build and run without it. The app converts to `Vector2` in `ToRaylib` (`src/app/RaylibConvert.h`), right before drawing.
  - Rejected: a math library such as glm. It is far bigger than the five functions I need.
- **A free function over a `Motion` struct, instead of a `Player` class with an `Update` method.** Data and behaviour stay apart, so the same function moves the player now and enemies later. This is the "small components and systems" shape described in the [roadmap](ROADMAP.md#architecture-goals).
- **Walls cancel only the velocity that points into them.** If the velocity stayed at full speed while the player is pinned to a wall, walking away would first have to brake that invisible speed, and the wall would feel sticky. Cancelling only that part also lets the player slide along the wall.
  - Rejected: stopping the mover completely on contact. Running along a wall, which happens all the time in an arena, would stutter.
- **A radial deadzone for the stick.** It looks at the length of the stick vector, so a diagonal stays a diagonal. The output is rescaled to start at zero at the edge of the deadzone, so there is no jump in speed when the stick leaves it.
  - Rejected: a deadzone per axis. It snaps slightly diagonal pushes to the four main directions, which feels like walking on rails.
- **Keyboard and stick add up instead of one overriding the other.** There is no "active device" state to get wrong, and the clamp in `StepMovement` keeps the sum from being faster than one device.
- **Input is read inside the fixed step.** `IsKeyDown` reports the current state of a key, so reading it in each step is correct and keeps the input next to the code that uses it. Key presses (an attack) will need a different approach on day 4, because a press can fall between two steps.

## How to tune it

The movement numbers are the defaults in `MovementStats` (`src/core/Movement.h`):

| Field | Default | Effect |
|---|---|---|
| `maxSpeed` | 300 units/s | Top speed. One world unit is one pixel at camera zoom 1. |
| `acceleration` | 3000 units/s^2 | Time to top speed is `maxSpeed / acceleration` (0.1 s). |
| `deceleration` | 4000 units/s^2 | Time to stop from top speed is `maxSpeed / deceleration` (0.075 s). |

A very high value (for example 100000) gives instant start or stop. These values move to JSON on day 6.

Other values:

| Value | Where | Default |
|---|---|---|
| Player body radius | `Player::radius` (`src/core/Player.h`) | 20 units |
| Stick deadzone | `kStickDeadzone` (`src/app/PlayerInput.cpp`) | 0.2 |
| Arena rectangle | `kArena` (`src/app/Game.cpp`) | (40, 60) to (1240, 680) |

## Tests

- `tests/core/Vec2Tests.cpp`: arithmetic, length, `ClampLength`, `MoveTowards` (no overshoot, zero distance) and `Lerp`.
- `tests/core/MovementTests.cpp`: `MotionAt` spawns a still mover with both positions set, a mover at rest stays still, acceleration reaches top speed and stays there, a diagonal is not faster than a straight line, half stick input gives half speed, releasing stops without sliding back, reversing direction passes through zero, and the step saves the previous position.
- `tests/core/ArenaTests.cpp`: the arena centre, a mover inside is left alone, a side wall cancels only the push into it, velocity away from a wall is kept, the top and bottom walls, a corner, and an arena narrower than the mover.
- `tests/core/AnalogInputTests.cpp`: drift inside the deadzone reads as zero, values past it are rescaled from zero, full tilt stays at 1, a diagonal keeps its direction, and a gamepad corner longer than 1 is capped.

The tests use round numbers that are exact in binary floating point where they can, and `doctest::Approx` where a square root is involved. `ReadMoveInput` and the drawing code are not unit tested, because they need a window and real devices. I test them by playing.

## Limitations

- Reversing direction uses `acceleration`, not `deceleration`, because there is input. A sharp turn is therefore a little slower than a full stop. If that feels sluggish in play, the fix is to use `deceleration` while the input points against the velocity.
- Walls are the only thing that stops a mover. Collisions between bodies come with the first enemy on day 3.
- Only the first gamepad is read, and only its left stick.
