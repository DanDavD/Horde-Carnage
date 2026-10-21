# Camera

## What it is

The camera that follows the player around an arena bigger than the screen. It chases the player smoothly, looks a little ahead in the direction the player is moving, and stops at the walls so the view does not wander into empty space.

The rules live in `horde_core` and know nothing about raylib:

- `src/core/FollowCamera.h`, `src/core/FollowCamera.cpp`: the `FollowCamera` component, the `FollowCameraStats` tuning values, `FollowCameraAt` to create one, `StepFollowCamera` to move it, and `ClampViewToArena`.

The game executable turns it into a raylib `Camera2D`:

- `src/app/Game.cpp`: `ToRaylibCamera` builds the `Camera2D`, and `Draw` wraps the arena and the player in `BeginMode2D` and `EndMode2D`. The HUD (FPS, version, controls, the Debug overlay text) is drawn after `EndMode2D`, so it stays fixed on screen.

## How it works

The camera has a `center`, a `previousCenter` and a `viewSize` (how much of the world fits on screen; at zoom 1 it is the window size, 1280x720). It is created with `FollowCameraAt`, already settled on the player, so the first frame does not slide in from somewhere else.

Once per fixed step, after the player has moved, `StepFollowCamera` does three things:

1. **Pick a goal:** the player's position plus `velocity * lookAhead`. When the player runs right, the goal is to the right of the player, so the player sees more of where they are going. When the player stops, the velocity drops to zero and the goal slides back onto the player.
2. **Clamp the goal** with `ClampViewToArena`, against the arena grown by `wallMargin` on every side. The view can show the walls and a strip of the outside, but not more. On an axis where the arena is smaller than the view, the view is centred on the arena.
3. **Move toward the goal** by the fraction `1 - exp(-sharpness * dt)` of the remaining distance, after saving the old center in `previousCenter`.

The renderer centres the `Camera2D` on `Lerp(previousCenter, center, alpha)`, the same interpolation the player uses.

## Design decisions

- **Exponential follow (close a fixed fraction of the gap each step).** It starts fast when the player is far and eases in as it gets close, so the camera never jerks or overshoots. It is one number to tune.
  - Rejected: a camera locked to the player. It is exact, but every small change of direction shakes the whole screen.
  - Rejected: a spring (stiffness and damping). It can overshoot, needs two numbers that interact, and the extra bounce is not something this game needs.
  - Rejected: a dead zone (the camera only moves when the player leaves a box in the middle). It suits platformers, where you read the level ahead. In an arena full of enemies I want the player near the middle all the time.
- **`1 - exp(-sharpness * dt)` instead of a fixed fraction per step.** With a fixed step both give the same motion, but the exponential form keeps the same follow speed if I ever change the simulation rate, and `sharpness` has a unit (per second) that is easy to reason about.
- **Look-ahead along the velocity, not the facing.** Facing keeps its direction when the player stops, so a facing-based look-ahead would keep the player off-centre while standing still. The velocity goes to zero, so the view recentres by itself.
- **The look-ahead is tuned against the follow lag.** An exponential follow trails a target that moves at speed `v` by `v / sharpness`. At top speed that is 300 / 8 = 37.5 units, and the look-ahead adds 300 * 0.25 = 75, so the player ends up about 37 units behind the centre of the screen. A look-ahead below `1 / sharpness` seconds would leave the camera behind the player.
- **The goal is clamped, not the result.** The current center is always inside the allowed range, and a blend of two points inside a range stays inside it. So the camera eases into the wall and stops there, and is never pushed back after a step.
- **A margin past the walls.** Without it the edge of the screen would stop exactly on the edge of the arena. The walls are drawn just outside the arena, so they would never be on screen, and the player would bump into something invisible. With 64 units the wall and a strip of the outside are visible, and the controls text sits over the outside instead of over the floor.
- **The camera is stepped in the fixed update and interpolated like the player.** Stepping it once per frame instead would make its follow speed depend on the frame rate, and the tests could not pin it down. Stepping it at 60 Hz without interpolation would make the view jump once per step while the player moves smoothly, which shows up as jitter on a 165 Hz monitor.
- **The camera is a component plus free functions in `horde_core`, like `Motion`.** It is tested without a window, and screen shake on day 12 can be added as an offset on top without touching the follow logic.

## How to tune it

The numbers are the defaults in `FollowCameraStats` (`src/core/FollowCamera.h`):

| Field | Default | Effect |
|---|---|---|
| `sharpness` | 8 per second | How tightly the camera follows. Higher is tighter. After `1 / sharpness` seconds (0.125 s) it has closed 63% of a jump. |
| `lookAhead` | 0.25 s | How far ahead of the player the camera aims, as seconds of the player's velocity. Keep it above `1 / sharpness`, or the camera trails behind. |
| `wallMargin` | 64 units | How far past each wall the view may reach. |

Other values:

| Value | Where | Default |
|---|---|---|
| Arena rectangle | `kArena` (`src/app/Game.cpp`) | (0, 0) to (2400, 1600) |
| View size | `kViewSize` (`src/app/Game.cpp`) | The window size, 1280x720 |

In a Debug build, `F1` shows the camera center in the overlay, and an orange cross where the camera is centred. The gap between the cross and the player is the look-ahead minus the follow lag.

## Tests

- `tests/core/FollowCameraTests.cpp`: a view inside the arena is left alone, a view near a wall stops where the screen edge meets the wall, an arena smaller than the view centres it, a new camera starts settled on its target, one step closes exactly `1 - exp(-sharpness * dt)` of the gap, the camera settles on a target that stops, it settles `velocity * lookAhead` ahead of a moving target, the look-ahead never pushes the view past a wall on any step, and the wall margin lets the view reach past the wall.

Most tests set `wallMargin` to 0 so the expected centers are easy to read. The margin has its own test.

`ToRaylibCamera` and the drawing are not unit tested, because they need a window. I check them by playing.

## Limitations

- Zoom is fixed at 1, so one world unit is one pixel. A zoom setting would scale `viewSize`, and nothing else in the camera would change.
- The camera follows only the player. A boss fight on day 11 may want it to frame both the player and the boss.
- The camera position is not rounded to whole pixels. Thin grid lines can shimmer at sub-pixel positions. I have not checked for it in motion yet, only in still screenshots. If it shows up, rounding the `Camera2D` target is the fix.
- The window is not resizable. If it becomes resizable, `viewSize` has to follow the window size.
