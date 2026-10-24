# Movement

## What it is

The rules for how anything in the arena moves: how fast it goes, how quickly it gets to that speed, how quickly it stops, which way it faces, and how the arena walls hold it in. The player uses it first. Enemies will use the same code from day 3.

The rules live in `horde_core`, so they have no idea where the input comes from or how the result is drawn:

- `src/core/Vec2.h`: a small 2D vector type with the few operations movement needs, plus `Rotated`, which the boss's Hair Ring uses to place its gap and its strands.
- `src/core/Movement.h`, `src/core/Movement.cpp`: the `Motion` component, the `MovementStats` tuning values, `MotionAt` to spawn a mover and the `StepMovement` function.
- `src/core/Arena.h`, `src/core/Arena.cpp`: the `Arena` rectangle and `ConfineToArena`, which keeps a mover inside its walls.
- `src/core/Facing.h`, `src/core/Facing.cpp`: `UpdateFacing`, which turns a facing direction toward the movement input when nothing aims. Aiming with the mouse (`AimFacing`) is described in [Aiming and weapons](WEAPONS.md).
- `src/core/AnalogInput.h`: `ApplyRadialDeadzone` for gamepad sticks.
- `src/core/Player.h`: the player's data (`Motion`, `MovementStats`, a body radius and a facing direction).

The game executable connects them to the hardware and the screen:

- `src/app/PlayerInput.h`, `src/app/PlayerInput.cpp`: `ReadMoveInput` reads the keyboard and the first gamepad.
- `src/app/Game.cpp`: passes the input to `StepSession`, which steps the world (`StepWorld` in `src/core/World.cpp`) every fixed step, draws the arena and the player, and in Debug builds draws a movement overlay.

## How it works

Every moving entity has a `Motion` component: `position`, `previousPosition` and `velocity`. It is created with `MotionAt(position)`, which sets both positions to the same point. Once per fixed step (1/60 s, see [Architecture](ARCHITECTURE.md)), this happens for the player:

1. **Read the input**: `Game::Update` calls `ReadMoveInput` and passes the result through `StepSession` to `StepWorld`, which does the rest of the steps. WASD, the arrow keys and the gamepad d-pad give `-1`, `0` or `1` on each axis. The left stick of the first gamepad goes through a radial deadzone of 0.2. All of them are added together.
2. **Turn the player.** With the mouse, `facing` points at the cursor (see [Aiming and weapons](WEAPONS.md)), whatever the movement does. Without an aim point, `UpdateFacing` makes `facing` the movement input direction at length 1, and keeps its last value when there is no input.
3. **Step the movement** with `StepMovement`:
   1. **Clamp the input to length 1.** The input says where the mover wants to go, and its length is how much of the top speed it wants. A keyboard diagonal is `(1, 1)`, which is about 1.41 long, so without the clamp the player would move 41% faster diagonally. An analog stick pushed halfway gives about 0.5, and keeps that half speed.
   2. **Move the velocity toward the target velocity** (`input * maxSpeed`) by at most `rate * dt`. The rate is `deceleration` when the mover is braking, and `acceleration` otherwise. Braking means there is no input, or the input points against the current velocity (their dot product is negative). A 90 degree turn is not braking. `MoveTowards` never goes past the target, so a mover that stops does not slide backwards.
   3. **Integrate the position** with the new velocity, after saving the old one in `previousPosition`.
4. **Confine it to the arena** with `ConfineToArena`. This happens after every body has moved and collisions have pushed overlapping bodies apart (see [Collisions](COLLISIONS.md)), so a body pushed into a wall is put back too. On each axis, if the body (a circle of `radius`) went through a wall, it is put back against the wall, and the part of the velocity that points into that wall is set to zero. The other axis is not touched, so the player slides along walls.

During a dash, the burst replaces `StepMovement` for the player. See [Player abilities](ABILITIES.md).

Enemies go through the same movement and wall steps with their own input and numbers. See [Enemies](ENEMIES.md).

The renderer draws the player at `Lerp(previousPosition, position, alpha)`, where `alpha` comes from `FixedTimestep::Alpha`. That keeps movement smooth on monitors faster than 60 Hz. A dark eye on the player's edge shows `facing`.

World units are pixels at camera zoom 1, and y grows downward like on screen. The arena is 2400x1600, bigger than the window, and a camera follows the player. See [Camera](CAMERA.md).

## Design decisions

- **Acceleration toward a target velocity, not instant velocity.** With instant velocity the player starts and stops in one frame, which is precise but looks weightless. A ramp of 0.1 s still feels instant in the hands and reads as weight on screen.
  - Rejected: physics with forces and friction (velocity decays by a factor each step). It never quite reaches zero or top speed, and the numbers are harder to tune by feel.
- **A separate deceleration rate.** Stopping a bit faster than starting makes dodging feel sharp. It is one extra number, and it can be set equal to `acceleration` if I do not want it.
- **Turning back counts as braking.** Pushing against the velocity uses `deceleration` until the velocity crosses zero, then `acceleration` takes over. Before this, turning around took longer than stopping (0.2 s against 0.075 s at the defaults), and dodging back the way you came felt sluggish. Now it takes 0.175 s.
  - Rejected: splitting the velocity into a part along the input and a part across it, and braking only the second. It handles curves more precisely, but the dot product test gives the same result for the turns that matter (straight back) with one line of code.
- **Without an aim, facing follows the input, not the velocity.** After a turn, the velocity still points the old way for a few steps, and the player should look where they are pushing now.
- **Aim feeds `facing` from another input, without touching the movement code.** On day 2 I planned a melee swing aimed by movement, and wrote that mouse aim could be added this way if needed. On day 4 I switched to guns aimed with the mouse, and it took exactly that: a second way to set `facing`.
- **Semi-implicit Euler** (update velocity, then move with the new velocity). With a fixed step and no springs it is stable and exact enough, and it is the simplest option.
- **My own `Vec2` instead of raylib's `Vector2`.** `horde_core` must not depend on raylib, so the tests build and run without it. The app converts to `Vector2` in `ToRaylib` (`src/app/RaylibConvert.h`), right before drawing.
  - Rejected: a math library such as glm. It is far bigger than the five functions I need.
- **A free function over a `Motion` struct, instead of a `Player` class with an `Update` method.** Data and behaviour stay apart, so the same function moves the player now and enemies later. This is the "small components and systems" shape described in the [roadmap](ROADMAP.md#architecture-goals).
- **Walls cancel only the velocity that points into them.** If the velocity stayed at full speed while the player is pinned to a wall, walking away would first have to brake that invisible speed, and the wall would feel sticky. Cancelling only that part also lets the player slide along the wall.
  - Rejected: stopping the mover completely on contact. Running along a wall, which happens all the time in an arena, would stutter.
- **A radial deadzone for the stick.** It looks at the length of the stick vector, so a diagonal stays a diagonal. The output is rescaled to start at zero at the edge of the deadzone, so there is no jump in speed when the stick leaves it.
  - Rejected: a deadzone per axis. It snaps slightly diagonal pushes to the four main directions, which feels like walking on rails.
- **Keyboard, d-pad and stick add up instead of one overriding the other.** There is no "active device" state to get wrong, and the clamp in `StepMovement` keeps the sum from being faster than one device.
- **A movement overlay in Debug builds only** (`F1`). It shows the input, speed, velocity, facing and position, and draws the velocity (green) and facing (yellow) as lines. I tune movement against these numbers instead of guessing. The code is inside `#ifndef NDEBUG`, so it does not exist in Release.
- **Input is read inside the fixed step.** `IsKeyDown` reports the current state of a key, so reading it in each step is correct and keeps the input next to the code that uses it. Key presses (an attack) will need a different approach on day 4, because a press can fall between two steps.

## How to tune it

The movement numbers are the defaults in `MovementStats` (`src/core/Movement.h`):

| Field | Default | Effect |
|---|---|---|
| `maxSpeed` | 300 units/s | Top speed. One world unit is one pixel at camera zoom 1. |
| `acceleration` | 3000 units/s^2 | Time to top speed is `maxSpeed / acceleration` (0.1 s). |
| `deceleration` | 4000 units/s^2 | Time to stop from top speed is `maxSpeed / deceleration` (0.075 s). Also used while turning back. |

A very high value (for example 100000) gives instant start or stop. These values may move to JSON with the boss's numbers on day 9.

Other values:

| Value | Where | Default |
|---|---|---|
| Player body radius | `Player::radius` (`src/core/Player.h`) | 20 units |
| Stick deadzone | `kStickDeadzone` (`src/app/PlayerInput.cpp`) | 0.2 |
| Arena rectangle | `kBossArena` (`src/core/World.h`) | (0, 0) to (1640, 1040) |
| Starting facing | `Player::facing` (`src/core/Player.h`), set by `CreateWorld` (`src/core/World.cpp`) | (0, 1), facing down; a new fight turns the player up, toward the boss |

## Tests

- `tests/core/Vec2Tests.cpp`: arithmetic, length, `Dot`, `Normalized` (including the zero vector), `ClampLength`, `MoveTowards` (no overshoot, zero distance), `Lerp`, and `Rotated` (clockwise on screen, length kept).

- `tests/core/MovementTests.cpp`: `MotionAt` spawns a still mover with both positions set, a mover at rest stays still, acceleration reaches top speed and stays there, a diagonal is not faster than a straight line, half stick input gives half speed, releasing stops without sliding back, turning back brakes at the deceleration rate and then accelerates the other way, a 90 degree turn uses the acceleration rate, and the step saves the previous position.
- `tests/core/ArenaTests.cpp`: the arena centre, a mover inside is left alone, a side wall cancels only the push into it, velocity away from a wall is kept, the top and bottom walls, a corner, and an arena narrower than the mover.
- `tests/core/FacingTests.cpp`: a new player faces down, input sets the facing at length 1, no input keeps the last facing, a keyboard diagonal faces diagonally, and a small stick push turns the player fully.
- `tests/core/AnalogInputTests.cpp`: drift inside the deadzone reads as zero, values past it are rescaled from zero, full tilt stays at 1, a diagonal keeps its direction, and a gamepad corner longer than 1 is capped.

The tests use round numbers that are exact in binary floating point where they can, and `doctest::Approx` where a square root is involved. `ReadMoveInput`, the drawing code and the Debug overlay are not unit tested, because they need a window and real devices. I test them by playing.

## Limitations

- Without the mouse, `facing` from the keyboard or the d-pad has 8 directions. Only the stick gives every angle.
- A turn of less than 90 degrees from the velocity uses `acceleration`, even though part of it is braking. It has not been tuned by hand yet, and it is the first thing to check if curves feel loose.
- Walls and other bodies are the only things that stop a mover. There are no obstacles inside the arena yet.
- Only the first gamepad is read, and only its left stick and d-pad.
