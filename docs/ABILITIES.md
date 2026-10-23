# Player abilities

## What it is

The moves that let the player survive a hard boss: for now the **dash**, a burst of speed that reads as a short teleport and makes the player untouchable while it lasts. It is the main defensive tool of the game, in the spirit of the roll in Dark Souls: you learn an attack, then dash through it at the right moment.

The rules live in `horde_core`:

- `src/core/Dash.h`, `src/core/Dash.cpp`: `DashStats`, the `Dash` component, `TryStartDash`, `StepDash` and `TickDashCooldown`.
- `src/core/World.cpp`: `StartDashIfAsked`, `MovePlayer`, and `HurtPlayer`, the single door through which the player takes damage.

The game executable reads Space or left Shift, draws the trail, and shows a recharge bar under the health bar.

## How it works

1. **Start.** When `dashPressed` is set and the dash is off cooldown, `TryStartDash` picks a direction: the movement input if there is one, otherwise the point the player aims at in this step, or the facing if the cursor is right on the player. It starts a burst of 0.12 s, an invulnerability window of 0.15 s, and a cooldown of 0.62 s (the burst plus 0.5 s), all counted from the start.
2. **Burst.** While dashing, `MovePlayer` does not call `StepMovement`. Each step, `StepDash` returns how far to move at 1500 units per second. The last step moves only for the time that was left, so a dash always covers exactly 180 units.
3. **Bodies and walls.** A dashing player is left out of the body collisions, so it passes through hairballs (and later the boss). The walls still apply, so a dash into a wall stops at it.
4. **Damage.** `HurtPlayer` ignores all damage while the dash's invulnerability lasts. Contact damage goes through it now; the boss's attacks will too.
5. **Guns.** The gun cannot fire during the burst. A click during it is still buffered by `PullTrigger` (the `canFire` parameter), so it fires as soon as the dash ends.
6. **After.** The player leaves the dash at running speed in the dash direction, and normal movement takes over.

On screen, four fading ghosts trail between where the dash started and the body. They fade with the invulnerability, so the trail also shows how long the player is safe. A thin bar under the health bar fills as the dash recharges and turns blue when it is ready.

## Design decisions

- **A real burst, not a teleport.** The body travels the distance in 7 or 8 steps, so walls stop it, the camera follows smoothly, and the player can never end up inside a wall. At 1500 units per second it is too fast to follow with the eye, so it still reads as a blink.
  - Rejected: an instant teleport. It needs its own rules for walls and for landing inside bodies, and it is harder to read for the player and for the boss.
- **Invulnerable for the whole burst, plus 0.03 s.** Dodging through an attack is the skill the boss will test. The extra fraction forgives landing right at the edge of an attack.
- **Through bodies, not into them.** A dash that bounces off the hairball in front of you is useless as an escape. Passing through bodies is what makes it a way out of a crowd or past the boss.
- **No shooting during the burst, but no lost clicks.** Attacking and dodging at once would make the dash an attack too. Buffering the click keeps the fire button responsive.
- **Standing still, the dash goes toward the aim of this step, not toward `facing`.** The facing is updated after moving, so it is one step old. A test found that a dash on the very first step went the wrong way.
- **Leave at running speed.** Keeping the burst's speed after it ends would slide the player for a quarter of a second. Running speed hands control straight back to the movement rules.
- **One door for damage.** `HurtPlayer` is the only place the player is hurt, so the dash protects against every damage source added later without each of them checking it.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Distance | `DashStats::distance` (`src/core/Dash.h`) | 180 units |
| Burst | `DashStats::duration` | 0.12 s |
| Invulnerability | `DashStats::invulnerability` | 0.15 s |
| Cooldown after the burst | `DashStats::cooldown` | 0.5 s |
| Keys | `CollectButtonPresses` (`src/app/PlayerInput.cpp`) | Space, left Shift |

In a Debug build, `F1` shows the dash cooldown and the invulnerability left.

## Tests

- `tests/core/DashTests.cpp`: a dash starts toward its direction at length 1; no direction starts nothing; a dash covers exactly its distance even when the burst is not a whole number of steps; the invulnerability outlasts the burst a little, then ends; the next dash waits for the cooldown; and not dashing moves nothing.
- `tests/core/WeaponTests.cpp`: a trigger that cannot fire still remembers a press for when it can.
- `tests/core/WorldTests.cpp`: a dash carries the player its distance and leaves it at running speed; with no movement input it goes where the player aims, or where it faces if the cursor is on the player; it stops at a wall; it goes through a hairball instead of shoving it; a dashing player cannot be hurt; and the gun does not fire during a dash, but a click in it fires right after.

The trail and the bar are not unit tested, because they need a window. I checked them in a Release build: the ghosts appear behind the player during the dash, and the bar refills under the health bar.

## Limitations

- **No gamepad button for the dash yet.** It comes with twin-stick support.
- **The dash does not refill on anything.** If the boss fight needs a more generous rhythm, a second charge is one number away.
