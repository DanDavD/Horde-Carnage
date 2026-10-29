# Aiming and weapons

## What it is

How the player aims and fires. The player moves with WASD and aims with the mouse, independently, like a twin-stick shooter (SAS: Zombie Assault 4, the "Dead Ops Arcade" mode of Call of Duty: Black Ops). They carry two guns: a semi-automatic pistol that fires once per click, and an automatic SMG that fires while the button is held. The gun in hand is shown in a panel in the bottom right corner, and Q, the keys 1 and 2, or the mouse wheel switch guns.

This document covers aiming, the trigger and switching. What a bullet does after it leaves the gun is in [Bullets and hits](BULLETS.md).

The rules live in `horde_core`:

- `src/core/PlayerCommands.h`: `PlayerCommands`, what the player asks for in one step (move, aim point, fire, weapon switch).
- `src/core/Weapon.h`, `src/core/Weapon.cpp`: `FireMode`, `WeaponStats` with the two starting guns (`kPistol`, `kSmg`), the `Weapon` component and `PullTrigger`.
- `src/core/Loadout.h`, `src/core/Loadout.cpp`: the two slots, which one is in hand, `SelectSlot`, `CycleWeapon` and `TickLoadout`.
- `src/core/Facing.h`, `src/core/Facing.cpp`: `AimFacing` turns the player toward the aim point.
- `src/core/LaserSight.h`, `src/core/LaserSight.cpp`: `CanProjectLaserSight`, `TraceLaserSight` and `TraceWorldLaserSight`, which clip a ray from `MuzzlePoint` against living targets, arena walls and the equipped gun's range.
- `src/core/World.cpp`: `StepWorld` aims, switches and pulls the trigger every fixed step after collisions settle.

The game executable reads the hardware and draws:

- `src/app/PlayerInput.cpp`: `ReadFireHeld` and `CollectButtonPresses` (left click, 1, 2, Q, mouse wheel, plus gamepad triggers and buttons).
- `src/app/Game.cpp`: sends the mouse's screen offset (`aimInView`) or the gamepad's unit heading (`aimDirection`) in `PlayerCommands`, and passes the live interpolated aim to `DrawWorld`.
- `src/app/render/PlayerDraw.cpp`, `WorldDraw.cpp` and `Hud.cpp`: draw the gun barrel, the crimson laser sight, the muzzle flash, the crosshair and the weapon panel.

## How it works

**Commands.** Every fixed step, `Game::Update` fills a `PlayerCommands` and hands it to `StepSession`:

- `move` comes from the keyboard (and the gamepad, as before).
- `aimInView` (on mouse) is the crosshair's offset from the view centre in game pixels, resolved inside `StepWorld` against `NextFollowCameraCenter` of the same step so camera look-ahead never lags the shot behind a strafing hunter.
- `aimDirection` (on gamepad) is the unit heading from `StepGamepadAim`, applied after movement so horizontal movement never skews the shot sideways, with `GamepadAim::rawDirection` kept separate so holding fire never compounds soft aim assist onto itself.
- `aimAt` remains available for world-space callers and unit tests.
- `fireHeld` is whether the fire button or trigger is down now.
- `firePressed`, `selectSlot` and `cycleWeapon` are presses. They are events, so `Game::Frame` collects them once per frame into `ButtonPresses`, and the first fixed step after that uses and clears them. A frame that runs no step keeps them; a frame that runs two steps does not use them twice.

**Aim.** After bodies move and the collision passes settle, `AimPlayer` turns `facing` along `aimDirection`, toward `NextFollowCameraCenter + *aimInView`, toward `aimAt`, or along `move`. On high-refresh monitors, `Game::DrawScene` also computes the live interpolated aim between fixed steps so the barrel, the laser sight and the crosshair stay collinear on every rendered frame.

**Trigger.** `HandleWeapons` (in `World.cpp`) switches guns if asked, ticks the timers of both guns, then calls `PullTrigger` on the gun in hand:

1. A press opens a buffer of `kTriggerBufferSeconds` (0.15 s).
2. A semi-automatic gun wants to fire when there is a buffered press. An automatic gun also wants to fire while the button is held.
3. If something else has the player's hands (a dash or a drink, see [Player abilities](ABILITIES.md)), `canFire` is false and it stops here, with the press still buffered.
4. If the gun wants to fire and its cooldown is over, it fires: the cooldown is set to `1 / shotsPerSecond`, the buffer is cleared, and the muzzle flash starts (0.05 s).

**Switching.** `SelectSlot` puts a slot in hand; `CycleWeapon` goes to the next one. Switching is instant. Both guns keep cooling down while in the holster, and a press buffered on the gun being put away is dropped.

**On screen.** While playing, the system cursor is locked to the game and hidden, and a crosshair is drawn instead. The crosshair starts where the cursor was and follows the mouse's movement times the mouse sensitivity (`MoveMouseAim`, see [Settings](SETTINGS.md)), stopping at the edges of the view; in the menus the cursor comes back where the crosshair was. In a browser the page does the same with the pointer lock (`web/shell.html`). The hunter holds the gun in both hands toward the aim, the pistol out in front and the SMG like a rifle, with a thin crimson laser sight leaving `MuzzlePoint` along the barrel (`DrawLaserSight`): it stops with a bright dot on the nearest living target or wall, reaches the exact range of the gun in open space (750 units for the pistol, 600 for the SMG) with a range pip at its tip, and disappears while dashing, drinking or dead. A yellow flash shows at the muzzle after a shot, and the other gun hangs on the hunter (see [Art direction](ART.md)). The panel in the bottom right corner shows both slots with their number, name and fire mode, and lights the one in hand.

## Design decisions

- **Aim is independent of movement.** I first planned melee, aimed by the movement direction. I switched to guns aimed with the mouse because I want the feel of SAS 4 and Dead Ops Arcade: kiting a horde while shooting behind you. That needs to run one way and shoot another. The movement code did not change; only where `facing` comes from did.
- **The simulation receives commands, never the hardware.** `PlayerCommands` is plain data. The tests play with it (the determinism test aims and fires too), and a gamepad will fill the same fields later, right stick for `aimAt` and a trigger for fire, without changing the rules.
  - Rejected: reading the mouse inside `StepWorld`. It would tie the core to raylib and make every rule untestable without a window.
- **Presses are buffered for 0.15 s.** A click that arrives just before the pistol is ready fires as soon as it is, instead of being lost. Without the buffer, fast clicking feels like the gun ignores you.
  - Rejected: dropping early presses. It punishes clicking slightly too fast, which is exactly what a semi-automatic gun asks the player to do.
  - Rejected: queueing every press. Ten frantic clicks would turn into a burst of shots the player no longer wants.
- **Fire rates are whole numbers of steps.** At 60 Hz the pistol (6 per second) waits 10 steps and the SMG (10 per second) waits 6, so both are exact. A small tolerance stops float rounding from adding one extra step. I first let the cooldown carry its leftover below zero, so that any rate would average out exactly. Tracing it showed that a gun that had been idle fired its second shot one step early. Whole-step rates are simpler and right for every gun I plan.
- **Two guns that do the same damage a second, in different shapes.** Both do 60 a second at full rate: the pistol in big hits, the SMG in many small ones. Neither is better; the boss's shield is what makes one or the other the right gun, the Shell for the pistol and the Barrier for the SMG (see [The boss's shield](SHIELD.md)).
- **Switching is instant, and each gun keeps its own cooldown.** A swap delay is a balancing tool; I would rather add it later, if switching to dodge the pistol's cooldown turns out to be a problem.
- **The guns are constant data for now.** `kPistol` and `kSmg` are `constexpr` values. Day 9 loads the same fields from JSON.
- **A drawn crosshair instead of the system cursor.** It is easier to find in a crowd, and it matches the art of the game.
- **The cursor is locked, and the crosshair follows its movement.** A crosshair tied to the cursor's position could not be faster or slower than the cursor, so there could be no mouse sensitivity, and in a window the cursor could slip out of the game mid-fight. Locked, the mouse never reaches the edge of the screen, and raylib reports how far it moved.
  - Rejected: scaling the cursor's position around the centre of the view. The cursor would still stop at the edge of the screen before the crosshair reached its side.
- **Either direction of the mouse wheel means "the other gun".** With two guns there is nothing else it could mean.
- **Aim and fire run after collisions, against this step's camera.** Unprojecting the mouse with last step's camera before movement caused shots to lag up to 19 world units behind the crosshair when reversing horizontal strafe (`A` to `D`), because the camera's 0.25 s velocity look-ahead swings 150 units across a reversal. Resolving `aimInView` against `NextFollowCameraCenter` after `SeparateEveryone` and `ConfineEveryone` sends every bullet straight through the crosshair without touching bullet speed or camera look-ahead.
  - Rejected: raising bullet speeds toward hitscan. Faster bullets change weapon balance and remove visible tracer streams without fixing the one-step camera lag.
- **A laser sight capped at the gun's range.** Tracing from `MuzzlePoint` through `SweepCircle` shows the exact line from the barrel through the crosshair, confirms when a target is in line of fire, and teaches the pistol's longer reach over the SMG the moment you swap guns.
  - Rejected: an infinite ray to the walls. It would promise hits at distances where SMG bullets vanish in mid-air.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Pistol fire rate and mode | `kPistol` (`src/core/Weapon.h`) | 6 per second, semi-automatic |
| SMG fire rate and mode | `kSmg` (`src/core/Weapon.h`) | 10 per second, automatic |
| Early press buffer | `kTriggerBufferSeconds` | 0.15 s |
| Muzzle flash | `kMuzzleFlashSeconds` | 0.05 s |
| Barrel length (where bullets start) | `WeaponStats::barrelLength` (`src/core/Weapon.h`) | 18 and 30 units |
| Damage, bullet speed, range, knockback | `WeaponStats` (`src/core/Weapon.h`) | See [Bullets and hits](BULLETS.md#how-to-tune-it) |
| Controls | `CollectButtonPresses`, `ReadFireHeld` (`src/app/PlayerInput.cpp`) | Left click; 1, 2, Q, wheel |

In a Debug build, `F1` shows the gun in hand, its cooldown, and the shots fired this run.

## Tests

- `tests/core/WeaponTests.cpp`: the starting guns are a semi-automatic pistol and an automatic SMG; a press fires a ready gun and starts the cooldown and the flash; holding a semi-automatic trigger fires once; holding an automatic trigger fires exactly 10 shots in one second and 20 in two; clicking every step still fires only 6 times a second; a press 0.05 s early fires as soon as the gun is ready; a press 0.48 s early is dropped; the timers never go below zero; and no press and no button means no shot.
- `tests/core/LoadoutTests.cpp`: the player starts with the pistol in hand; selecting a slot; a slot that does not exist is ignored; cycling wraps around; a holstered gun keeps cooling down; and switching away drops a buffered press.
- `tests/core/FacingTests.cpp`: aiming turns the facing toward the target, and a target on top of the player keeps the last facing.
- `tests/core/LaserSightTests.cpp`: `CanProjectLaserSight` turns off while dashing, drinking or dead; `TraceWorldLaserSight` caps at 750 units for the pistol and 600 for the SMG in open space; `TraceLaserSight` clips on all four walls and handles a barrel poking into a wall; and the beam stops on the nearest living hairball or solid boss while passing through dead hairballs and an airborne leaping boss.
- `tests/core/WorldTests.cpp`: the player faces the aim point while walking another way; shots aimed through `aimInView` pass straight through the camera's crosshair while strafing and reversing; shots aimed with `aimDirection` hold their exact heading while strafing horizontally; the fire button fires the gun in hand; holding the button fires the SMG 10 times in a second and the pistol once; weapon commands switch the gun; and the determinism test now aims, fires and switches too.

The crosshair, the barrel, the flash and the panel are not unit tested, because they need a window. I checked them in Release and Debug builds: the barrel points at the crosshair while the player walks the other way, the SMG fired 10 shots in one second of holding, and the pistol fired once.

## Limitations

- **Fire rates that are not a whole number of steps are rounded.** 7 shots per second would become 6.67. None of the planned guns needs that.
- **The mouse wheel cycles once per frame it moves.** A touchpad that scrolls for several frames can switch more than once.
