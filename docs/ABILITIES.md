# Player abilities

## What it is

The two moves that let the player survive a hard boss, both in the spirit of Dark Souls:

- **The dash**, a burst of speed that reads as a short teleport and makes the player untouchable while it lasts. It is the main defensive tool: you learn an attack, then dash through it at the right moment.
- **The flasks**, three heals per attempt. Drinking takes a second, slows you down and leaves you unable to dash or shoot, so healing in the wrong moment gets punished.

The rules live in `horde_core`:

- `src/core/Dash.h`, `src/core/Dash.cpp`: `DashStats`, the `Dash` component, `TryStartDash`, `StepDash` and `TickDashCooldown`.
- `src/core/Flasks.h`, `src/core/Flasks.cpp`: `FlaskStats`, the `Flasks` component, `TryStartDrinking`, `StepFlasks` and `InterruptDrinking`.
- `src/core/Health.h`, `src/core/Health.cpp`: `Heal`, which restores health up to the maximum.
- `src/core/World.cpp`: `StartDashIfAsked`, `StartDrinkIfAsked`, `DrinkFlasks`, `MovePlayer`, and `HurtPlayer`, the single door through which the player takes damage.

The game executable reads Space or left Shift for the dash and E for a flask. It draws the dash trail, a recharge bar under the health bar, one bottle icon per flask next to the health bar, and a ring that fills around the player while drinking.

## How it works

### Dash

1. **Start.** When `dashPressed` is set, the dash is off cooldown and the player is not drinking, `TryStartDash` picks a direction: the movement input if there is one, otherwise the point the player aims at in this step, or the facing if the cursor is right on the player. It starts a burst of 0.12 s, an invulnerability window of 0.15 s, and a cooldown of 0.62 s (the burst plus 0.5 s), all counted from the start.
2. **Burst.** While dashing, `MovePlayer` does not call `StepMovement`. Each step, `StepDash` returns how far to move at 1500 units per second. The last step moves only for the time that was left, so a dash always covers exactly 180 units.
3. **Bodies and walls.** A dashing player is left out of the body collisions, so it passes through hairballs (and later the boss). The walls still apply, so a dash into a wall stops at it.
4. **Damage.** `HurtPlayer` ignores all damage while the dash's invulnerability lasts. Contact damage goes through it now; the boss's attacks will too.
5. **Guns.** The gun cannot fire during the burst. A click during it is still buffered by `PullTrigger` (the `canFire` parameter), so it fires as soon as the dash ends.
6. **After.** The player leaves the dash at running speed in the dash direction, and normal movement takes over.

On screen, four fading ghosts trail between where the dash started and the body. They fade with the invulnerability, so the trail also shows how long the player is safe. A thin bar under the health bar fills as the dash recharges and turns blue when it is ready.

### Flasks

1. **Start.** When `healPressed` is set, the player is hurt, not dashing and has a charge left, `TryStartDrinking` spends the charge at once and starts a 1 s drink.
2. **Drinking.** Each step, `StepFlasks` counts the drink down. Meanwhile `MovePlayer` caps the player at 35% of top speed, the dash cannot start, and the gun cannot fire.
3. **The heal.** In the step the drink finishes, `StepFlasks` returns 40, and `Heal` restores that much health, never above the maximum.
4. **Getting hit.** When `HurtPlayer` lands a hit, it calls `InterruptDrinking`: the drink stops, nothing is healed, and the charge stays spent.

On screen, three bottles next to the health bar show the charges left, and an orange ring fills clockwise around the player during a drink.

## Design decisions

### Dash

- **A real burst, not a teleport.** The body travels the distance in 7 or 8 steps, so walls stop it, the camera follows smoothly, and the player can never end up inside a wall. At 1500 units per second it is too fast to follow with the eye, so it still reads as a blink.
  - Rejected: an instant teleport. It needs its own rules for walls and for landing inside bodies, and it is harder to read for the player and for the boss.
- **Invulnerable for the whole burst, plus 0.03 s.** Dodging through an attack is the skill the boss will test. The extra fraction forgives landing right at the edge of an attack.
- **Through bodies, not into them.** A dash that bounces off the hairball in front of you is useless as an escape. Passing through bodies is what makes it a way out of a crowd or past the boss.
- **No shooting during the burst, but no lost clicks.** Attacking and dodging at once would make the dash an attack too. Buffering the click keeps the fire button responsive.
- **Standing still, the dash goes toward the aim of this step, not toward `facing`.** The facing is updated after moving, so it is one step old. A test found that a dash on the very first step went the wrong way.
- **Leave at running speed.** Keeping the burst's speed after it ends would slide the player for a quarter of a second. Running speed hands control straight back to the movement rules.
- **One door for damage.** `HurtPlayer` is the only place the player is hurt, so the dash protects against every damage source added later, and a hit interrupts a drink, without each damage source checking either.

### Flasks

- **Few charges and a slow drink, instead of health pickups.** A boss fight in this style is about learning, not about farming health. Three flasks per attempt make every heal a decision, and the second of drinking is what makes the decision risky: the boss can punish it.
  - Rejected: health dropped by enemies. It rewards killing adds over learning the boss, and the fight would depend on luck.
- **The charge is spent at the start, and a hit wastes it.** That is what makes "heal now or not" a real question. If a hit only paused the drink, drinking in the middle of an attack would cost nothing.
- **The heal arrives at the end, all at once.** A heal over time during the drink would blur the moment it pays off. One clear moment matches the ring that fills up.
- **No drinking at full health.** It would only waste a flask on a stray key press.
- **The dash and the drink exclude each other.** You cannot cancel a drink into a dash to escape, and you cannot start a drink in the middle of a dash. The commitment is the point.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Dash distance | `DashStats::distance` (`src/core/Dash.h`) | 180 units |
| Dash burst | `DashStats::duration` | 0.12 s |
| Dash invulnerability | `DashStats::invulnerability` | 0.15 s |
| Dash cooldown after the burst | `DashStats::cooldown` | 0.5 s |
| Flask charges | `FlaskStats::maxCharges` (`src/core/Flasks.h`) | 3 |
| Drink time | `FlaskStats::drinkSeconds` | 1 s |
| Health per flask | `FlaskStats::heal` | 40 |
| Speed while drinking | `FlaskStats::moveSpeedFactor` | 35% |
| Keys | `CollectButtonPresses` (`src/app/PlayerInput.cpp`) | Space or left Shift to dash, E to drink |

In a Debug build, `F1` shows the dash cooldown, the dash invulnerability left, the flasks left and the drink time left.

## Tests

- `tests/core/DashTests.cpp`: a dash starts toward its direction at length 1; no direction starts nothing; a dash covers exactly its distance even when the burst is not a whole number of steps; the invulnerability outlasts the burst a little, then ends; the next dash waits for the cooldown; and not dashing moves nothing.
- `tests/core/FlasksTests.cpp`: new flasks are full; starting a drink spends a charge at once; no drink starts during another or with no charges; the heal arrives once, when the drink finishes; an interrupted drink heals nothing and keeps the charge spent; healing stops at the maximum; and the dead cannot be healed.
- `tests/core/WeaponTests.cpp`: a trigger that cannot fire still remembers a press for when it can.
- `tests/core/WorldTests.cpp`:
  - Dash: it carries the player its distance and leaves it at running speed; with no movement input it goes where the player aims, or where it faces if the cursor is on the player; it stops at a wall; it goes through a hairball instead of shoving it; a dashing player cannot be hurt; and the gun does not fire during a dash, but a click in it fires right after.
  - Flasks: a drink heals when it finishes and spends a flask; a drink at full health is refused; while drinking the player is slow and cannot dash or shoot; a hit while drinking wastes the flask; and the flasks run out after three drinks.

The trail, the bar, the bottles and the ring are not unit tested, because they need a window. I checked them in Release and Debug builds: the ghosts appear behind the player during a dash, the ring fills during a drink, the bottles go from three to two, and a hairball that caught the player in the middle of a drink left the flask spent and the health unchanged.

## Limitations

- **No gamepad buttons for the dash and the flasks yet.** They come with twin-stick support.
- **Nothing refills the flasks during an attempt.** A death and a restart refill them, like resting at a bonfire.
- **The dash does not refill on anything.** If the boss fight needs a more generous rhythm, a second charge is one number away.
