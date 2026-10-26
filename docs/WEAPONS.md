# Aiming and weapons

## What it is

How the player aims and fires. The player moves with WASD and aims with the mouse, independently, like a twin-stick shooter (SAS: Zombie Assault 4, the "Dead Ops Arcade" mode of Call of Duty: Black Ops). They carry two guns: a semi-automatic pistol that fires once per click, and an automatic SMG that fires while the button is held. The gun in hand is shown in a panel in the bottom right corner, and Q, the keys 1 and 2, or the mouse wheel switch guns.

This document covers aiming, the trigger and switching. What a bullet does after it leaves the gun is in [Bullets and hits](BULLETS.md).

The rules live in `horde_core`:

- `src/core/PlayerCommands.h`: `PlayerCommands`, what the player asks for in one step (move, aim point, fire, weapon switch).
- `src/core/Weapon.h`, `src/core/Weapon.cpp`: `FireMode`, `WeaponStats` with the two starting guns (`kPistol`, `kSmg`), the `Weapon` component and `PullTrigger`.
- `src/core/Loadout.h`, `src/core/Loadout.cpp`: the two slots, which one is in hand, `SelectSlot`, `CycleWeapon` and `TickLoadout`.
- `src/core/Facing.h`, `src/core/Facing.cpp`: `AimFacing` turns the player toward the aim point.
- `src/core/World.cpp`: `StepWorld` aims, switches and pulls the trigger every fixed step.

The game executable reads the hardware and draws:

- `src/app/PlayerInput.cpp`: `ReadFireHeld` and `CollectButtonPresses` (left click, 1, 2, Q, mouse wheel).
- `src/app/Game.cpp`: turns the mouse into a world point and builds the `PlayerCommands`.
- `src/app/render/PlayerDraw.cpp` and `Hud.cpp`: draw the gun barrel and the muzzle flash, and the crosshair and the weapon panel.

## How it works

**Commands.** Every fixed step, `Game::Update` fills a `PlayerCommands` and hands it to `StepSession`:

- `move` comes from the keyboard (and the gamepad, as before).
- `aimAt` is the mouse position turned into a world point with the camera (`GetScreenToWorld2D`).
- `fireHeld` is whether the left button is down now.
- `firePressed`, `selectSlot` and `cycleWeapon` are presses. They are events, so `Game::Run` collects them once per frame into `ButtonPresses`, and the first fixed step after that uses and clears them. A frame that runs no step keeps them; a frame that runs two steps does not use them twice.

**Aim.** After the player moves, `AimFacing` turns `facing` toward `aimAt`. If the cursor is within one unit of the player's centre the old facing is kept. Without an aim point (no mouse), `UpdateFacing` makes the player face where it walks, as before.

**Trigger.** `HandleWeapons` (in `World.cpp`) switches guns if asked, ticks the timers of both guns, then calls `PullTrigger` on the gun in hand:

1. A press opens a buffer of `kTriggerBufferSeconds` (0.15 s).
2. A semi-automatic gun wants to fire when there is a buffered press. An automatic gun also wants to fire while the button is held.
3. If something else has the player's hands (a dash or a drink, see [Player abilities](ABILITIES.md)), `canFire` is false and it stops here, with the press still buffered.
4. If the gun wants to fire and its cooldown is over, it fires: the cooldown is set to `1 / shotsPerSecond`, the buffer is cleared, and the muzzle flash starts (0.05 s).

**Switching.** `SelectSlot` puts a slot in hand; `CycleWeapon` goes to the next one. Switching is instant. Both guns keep cooling down while in the holster, and a press buffered on the gun being put away is dropped.

**On screen.** The system cursor is hidden while playing and a crosshair is drawn instead. The gun is a light barrel that sticks out of the body toward the aim, longer and thicker for the SMG, with a yellow flash at its tip after a shot. The panel in the bottom right corner shows both slots with their number, name and fire mode, and lights the one in hand.

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
- **Either direction of the mouse wheel means "the other gun".** With two guns there is nothing else it could mean.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Pistol fire rate and mode | `kPistol` (`src/core/Weapon.h`) | 6 per second, semi-automatic |
| SMG fire rate and mode | `kSmg` (`src/core/Weapon.h`) | 10 per second, automatic |
| Early press buffer | `kTriggerBufferSeconds` | 0.15 s |
| Muzzle flash | `kMuzzleFlashSeconds` | 0.05 s |
| Barrel length (where bullets start) | `WeaponStats::barrelLength` (`src/core/Weapon.h`) | 14 and 22 units |
| Damage, bullet speed, range, knockback | `WeaponStats` (`src/core/Weapon.h`) | See [Bullets and hits](BULLETS.md#how-to-tune-it) |
| Controls | `CollectButtonPresses`, `ReadFireHeld` (`src/app/PlayerInput.cpp`) | Left click; 1, 2, Q, wheel |

In a Debug build, `F1` shows the gun in hand, its cooldown, and the shots fired this run.

## Tests

- `tests/core/WeaponTests.cpp`: the starting guns are a semi-automatic pistol and an automatic SMG; a press fires a ready gun and starts the cooldown and the flash; holding a semi-automatic trigger fires once; holding an automatic trigger fires exactly 10 shots in one second and 20 in two; clicking every step still fires only 6 times a second; a press 0.05 s early fires as soon as the gun is ready; a press 0.48 s early is dropped; the timers never go below zero; and no press and no button means no shot.
- `tests/core/LoadoutTests.cpp`: the player starts with the pistol in hand; selecting a slot; a slot that does not exist is ignored; cycling wraps around; a holstered gun keeps cooling down; and switching away drops a buffered press.
- `tests/core/FacingTests.cpp`: aiming turns the facing toward the target, and a target on top of the player keeps the last facing.
- `tests/core/WorldTests.cpp`: the player faces the aim point while walking another way; the fire button fires the gun in hand; holding the button fires the SMG 10 times in a second and the pistol once; weapon commands switch the gun; and the determinism test now aims, fires and switches too.

The crosshair, the barrel, the flash and the panel are not unit tested, because they need a window. I checked them in Release and Debug builds: the barrel points at the crosshair while the player walks the other way, the SMG fired 10 shots in one second of holding, and the pistol fired once.

## Limitations

- **Fire rates that are not a whole number of steps are rounded.** 7 shots per second would become 6.67. None of the planned guns needs that.
- **The mouse is turned into a world point with the camera of the last step**, while the screen shows a camera interpolated a fraction of a step behind. The difference is well under a pixel.
- **The mouse wheel cycles once per frame it moves.** A touchpad that scrolls for several frames can switch more than once.
- **No gamepad aiming yet.** The aim always comes from the mouse, so with only a gamepad the player aims wherever the cursor was left. Twin-stick aiming (right stick to aim, a trigger to fire) comes later.
