# Controllers

## What it is

Gamepad support across the whole game: fighting with twin sticks (analog movement, a fixed-distance crosshair and soft aim assist toward the boss and her living hairballs), navigating menus with the stick and d-pad with repeat when held, pausing when a gamepad unplugs mid-fight, and detecting which controller is connected to show matching button glyphs and labels (Xbox, PlayStation, Nintendo Switch or generic).

The code:

- `src/core/AnalogInput.h`, `src/core/AnalogInput.cpp`: `ApplyRadialDeadzone`, which rescales stick inputs above the deadzone threshold from zero, preserving diagonal range.
- `src/core/GamepadAim.h`, `src/core/GamepadAim.cpp`: `StepGamepadAim`, `GamepadAimPoint`, maintaining the aim vector when releasing the stick, and `ApplyAimAssist` with a 48-degree cone toward the boss and hairballs.
- `src/core/GamepadConnection.h`, `src/core/GamepadConnection.cpp`: `GamepadConnection` and `StepGamepadConnection`, tracking four gamepad ports and reporting when the active controller disconnects.
- `src/core/MenuRepeat.h`, `src/core/MenuRepeat.cpp`: `MenuRepeat`, `LatchMenuRepeat` and `StepMenuRepeat`, turning a held stick or d-pad into discrete menu steps with hysteresis, initial delay, repeat cadence and dominant-axis selection.
- `src/core/GamepadType.h`, `src/core/GamepadType.cpp`: `GamepadType`, `ClassifyGamepadName`, `GamepadControlLabel`, `GamepadActionBinding` and `GamepadActionBindingSummary`, which classify device names into four families and map positional buttons and actions.
- `src/app/PlayerInput.h`, `src/app/PlayerInput.cpp`: reading raylib gamepad state, analog axes, buttons, triggers, detecting active devices and identifying controller models.
- `src/app/render/InputPrompts.h`, `src/app/render/InputPrompts.cpp`: drawing procedural badges in code for all four controller families plus keyboard keycaps and mouse glyphs.
- `src/app/render/MenuDraw.cpp`: drawing the controls table (`DrawControlsScreen`) and footer prompt bar across menus.

## How it works

**Twin-stick controls.**

| Action | Xbox | PlayStation | Switch | Generic |
|---|---|---|---|---|
| Move | Left stick or d-pad | Left stick or d-pad | Left stick or d-pad | Left stick or d-pad |
| Aim | Right stick | Right stick | Right stick | Right stick |
| Fire | RT (hold with SMG) | R2 (hold with SMG) | ZR (hold with SMG) | RT (hold with SMG) |
| Switch gun | Y | Triangle | X | Y |
| Dash | LT or RB | L2 or R1 | ZL or R | LT or RB |
| Drink a flask | X | Square | Y | X |
| Pause | Menu | Options | + | Start |
| Try again | A (on end screen) | Cross (on end screen) | B (on end screen) | A (on end screen) |

- **Movement.** The left stick passes through `ApplyRadialDeadzone` with a threshold of 0.18. Beyond the deadzone, magnitude rescales from zero to one, so nudging the stick starts at slow speed and full tilt gives full speed.
- **Aiming and crosshair.** The right stick pushes a crosshair held 260 world units ahead of the hunter, and `Game::Update` passes the resulting unit heading as `PlayerCommands::aimDirection` so strafing never skews shots sideways. Releasing the stick while holding the fire trigger (`RT`, `R2` or `ZR`) keeps the unassisted `GamepadAim::rawDirection` and recomputes soft aim assist from it each step rather than compounding onto `direction`, letting you kite backwards or circle-strafe freely without drift.
- **Soft aim assist.** `ApplyAimAssist` scans the boss and active hairballs within a 48-degree cone (0.418 radians half-angle). If a target sits inside the cone, the aim heading nudges toward it smoothly. The boss takes priority unless a living hairball is significantly closer.
- **Dual dash triggers.** Both the lower trigger (`LT`, `L2`, `ZL`) and the upper right shoulder (`RB`, `R1`, `R`) trigger a dash. Players coming from Souls-likes often prefer bumper dashes, while twin-stick shooter players prefer triggers; both work simultaneously.

**Menu navigation.**

The left stick and d-pad navigate all menus and settings screens:
- **Hysteresis.** The stick must cross 0.5 magnitude to engage and drops below 0.35 to release, avoiding jitter from thumb tremor.
- **Dominant-axis selection.** On slanted pushes, `StepMenuRepeat` picks whichever axis has the larger deflection, preventing accidental setting changes when moving up and down rows in Settings.
- **Cadence.** Steps immediately on first deflection, waits 0.38 s, and then repeats every 0.11 s while held.
- **Latching.** Opening a menu while a stick is already deflected latches the input so the cursor does not jump on the first frame.

**Hotplugging and disconnect pause.**

The game monitors up to four controller ports:
- Touching any connected gamepad immediately switches active input to that controller.
- Unplugging the active controller mid-fight immediately pauses the game (`SessionState::Paused`), preventing unfair deaths from a dead battery or loose cable.
- Plugging in a new gamepad never interrupts the game and works as soon as an input is pressed.

**Controller classification.**

`ClassifyGamepadName` parses the raw string from raylib's `GetGamepadName` (desktop GLFW or browser Gamepad API):
1. Brand and USB vendor tokens (`xbox`, `045e`, `playstation`, `dualshock`, `dualsense`, `sony`, `054c`, `nintendo`, `switch`, `joy-con`, `057e`) are checked first.
2. Windows DirectInput HID names (such as Sony's `"Wireless Controller"` or Nintendo's `"Pro Controller"`) are resolved without false positives on `"Xbox Wireless Controller"`.
3. Unrecognized controllers fall back cleanly to `GamepadType::Generic`.

**Procedural prompts drawn in code.**

Prompts are drawn with raylib 2D primitives in `InputPrompts.cpp`:
- **Xbox**: dark iron caps with jewel-toned accent rims (`kXboxA` green, `kXboxB` red, `kXboxX` blue, `kXboxY` amber), lit trigger lips and circular 3-line Menu icons.
- **PlayStation**: geometric vector strokes (Cross, Circle, Square, Triangle), `L2`/`R2`/`R1` shoulder badges and vertical Options capsules.
- **Switch**: bone-rimmed caps with Nintendo's physical swapped layout (bottom `B`, right `A`, left `Y`, top `X`), `ZL`/`ZR`/`R` badges and circular `+` icons.
- **Generic**: neutral monochrome iron-and-bone caps (`A`/`B`/`X`/`Y`), `LT`/`RT`/`RB` badges and Start pills with play triangles.
- **Keyboard and mouse**: forged beveled keycaps (`WASD`, `SPACE`, `ENTER`, `ESC`), mouse body with highlighted left-button quadrant for firing, and mouse scroll wheel.

**Dynamic controls screen.**

The table in *Settings > Controls* adapts its third column (`GAMEPAD`):
- When playing with keyboard and mouse, it displays multi-platform notation (`RT / R2 / ZR`, `Y / Triangle`, `LT or RB (L2 / R1)`, `X / Square`).
- When a gamepad is active, it displays the exact labels for the connected family (`Triangle` on PlayStation, `X` on Switch, `Y` on Xbox).

## Design decisions

- **Soft aim assist cone instead of hard snap.** Twin-stick shooting without mouse precision can feel frustrating against small moving hairballs. A 48-degree soft cone nudges bullets toward targets without removing player agency or preventing manual aiming.
- **Fixed-distance crosshair.** A thumbstick reports an angle and magnitude rather than an absolute screen coordinate. Holding the crosshair 260 units ahead gives direct visual feedback of the bullet vector.
- **Aim lock while holding fire.** Releasing the right stick while holding fire keeps the last valid aiming angle. This allows comfortable kiting without thumb fatigue.
- **Dual dash buttons.** Allowing both `LT` and `RB` lets players use whichever grip feels natural without needing a button remapping screen.
- **Procedural prompts in code.** Keeps the game 100% procedural with zero texture asset files, preserves the dark crypt aesthetic, and adds nothing to the web build download.
- **Pure logic in `horde_core`.** Radial deadzones, aim assist vectors, connection state machines, menu repeat timers and name classification are tested without window or input dependencies.

## How to tune it

| Parameter | Where | Value |
|---|---|---|
| Stick radial deadzone | `kStickDeadzone` (`AnalogInput.h`) | 0.18 |
| Crosshair distance | `kGamepadCrosshairDistance` (`GamepadAim.h`) | 260 world units |
| Aim assist cone angle | `kAimAssistAngle` (`GamepadAim.h`) | 48 degrees (0.418 rad half-angle) |
| Menu repeat thresholds | `MenuRepeatStats` (`MenuRepeat.h`) | Engage 0.5, release 0.35 |
| Menu repeat timing | `MenuRepeatStats` (`MenuRepeat.h`) | Initial delay 0.38 s, repeat 0.11 s |
| Prompt cap dimensions | `InputPrompts.cpp` | Face 20 px, shoulders 17.5 px, keycaps 18 px |

## Tests

- `tests/core/AnalogInputTests.cpp`: radial deadzone rescaling from zero, preserving diagonal range, and clamping inputs at magnitude 1.
- `tests/core/GamepadAimTests.cpp`: deadzone filtering, retaining aim heading on stick release, holding aim while firing, and aim assist cone targeting priority.
- `tests/core/GamepadConnectionTests.cpp`: initial slot scanning, hotplugging controllers across slots, active slot switching on input, and disconnect detection.
- `tests/core/GamepadTypeTests.cpp`: classification of Xbox, PlayStation, Switch and Generic names, positional button labels, action bindings per family, and summary strings.
- `tests/core/MenuRepeatTests.cpp`: initial delay, steady repeat cadence, hysteresis against tremor, dominant-axis selection, and open latching.

## Limitations

- **No rumble on desktop raylib.** Desktop GLFW's `SetGamepadVibration` in raylib 5.5 is currently a stub on Windows.
- **Steam Controller.** Outside of Steam Input (which exposes it as an Xbox controller), custom Steam Controller hardware is not natively detected.
