# Menus

## What it is

The screens around the fight: the title, the pause menu, the settings and the controls, and the choices on the end screens. They share one look, taken from the games this one borrows from: a few words in capitals on the dark, grey when they are not selected, and the selected one in bone between a thin line and a small diamond on each side, in the manner of Dark Souls and Hollow Knight. There are no boxes and no buttons to draw, only the words. Color is kept for what means something: the red of "YOU DIED" and the gold of "VICTORY".

The code:

- `src/core/Menu.h`, `src/core/Menu.cpp`: `Menu`, `MenuInput` and `StepMenu`, the selection and what one frame of input does to it.
- `src/core/MenuRepeat.h`, `src/core/MenuRepeat.cpp`: `MenuRepeat`, `LatchMenuRepeat` and `StepMenuRepeat`, which turn a held left stick or d-pad into discrete menu steps with hysteresis and key-like repeat.
- `src/core/GamepadConnection.h`, `src/core/GamepadConnection.cpp`: `GamepadConnection` and `StepGamepadConnection`, which pick up newly plugged-in controllers across the four slots and report when the active one unplugs.
- `src/core/GamepadType.h`, `src/core/GamepadType.cpp`: `GamepadType`, `ClassifyGamepadName` and `GamepadControlLabel`, which classify a controller's raw platform name into `Xbox`, `PlayStation`, `Switch` or `Generic` and name its positional buttons.
- `src/core/Session.h`, `src/core/Session.cpp`: the `Title` and `Paused` states, `ReturnToTitle`, `BeginFight`, `PauseFight` and `ResumeFight` (see [Game flow](GAME_FLOW.md)).
- `src/app/render/MenuDraw.h`, `src/app/render/MenuDraw.cpp`: the pages (`MenuPage`), the items of each, where they sit, which one the mouse is over, and how they are drawn; the title, pause, settings and controls screens.
- `src/app/render/InputPrompts.h`, `src/app/render/InputPrompts.cpp`: `DrawMenuFooter` and `DrawHudPrompts`, which draw procedural keyboard keycaps, mouse icons and gamepad badges for all four controller families.
- `src/app/render/Backdrop.h`, `src/app/render/Backdrop.cpp`: the blurred still of the fight behind the pause menu.
- `src/app/PlayerInput.cpp`: `ReadMenuInput`, `ReadPadMenuDirection`, `ReadGamepadPresence`, `FindActiveGamepadSlot`, `ReadPausePressed`, `ReadQuickRetryPressed` and, in a browser, `TakePointerLockLost`.
- `src/app/Game.cpp`: `HandleMenus`, `Choose` and `GoBack`, which turn a choice into a change of state or of page.

## How it works

**The screens.**

| Screen | Items | Back (Esc, B) |
|---|---|---|
| Title | Begin, Settings, Quit (no Quit in a browser) | Nothing |
| Pause | Resume, Settings, Return to title | Resume |
| Settings | A line per setting, Controls, Back | Back to the title or the pause, on Settings |
| Controls | Back | Back to Settings, on Controls |
| YOU DIED | Try again, Return to title | Nothing |
| VICTORY | Fight again, Return to title | Nothing |

The game opens on the title, over a fresh world that waits behind it, darkened. The game's name and its menu sit together in the middle of the screen. Esc (or Start on a gamepad, or unplugging the active gamepad) pauses a fight. Settings and Controls open over the title or the pause, which stay in their state behind them; they are described in [Settings](SETTINGS.md). The end screens' choices sit in a row at the foot of the dark band, and stay dim until the restart delay has passed. R still retries at once after a death or a victory, for the next attempt without the menu.

**Choosing.** `StepMenu` takes one frame of input from anything: W, S, up and down (or the left stick and d-pad) move. A, D, left and right (or the left stick and d-pad) ask to change the selected item's value, which only the settings have; in a row of choices, such as the end screens', `Game` turns them into moves along the row. Holding left or right on the keyboard, or holding any direction on the left stick or d-pad, waits 0.38 s after the first step and then repeats every 0.11 s; `StepMenuRepeat` picks the dominant axis so a slanted push in Settings never changes a value while moving to another row, and `LatchMenuRepeat` suppresses an opening jump if the stick was already held when the menu opened. Enter or Space (or A) confirm, and Esc or Backspace (or B) go back. The mouse selects the item it moves onto and a click chooses the item under it. A mouse that has not moved does not take the selection, so a player using the keys is not pulled back to wherever the mouse rests. The selection wraps round at both ends. Going back wins over everything else in the same frame. Each move plays a dry tick and each choice a low knock, both synthesized like the rest of the sounds (see [Audio](AUDIO.md)).

**The marks.** When the selection moves, the two lines beside the new item slide out over 0.12 s from its diamonds. It is the only animation in the menus.

**The prompt bar.** Every menu screen carries a centered bar of prompts at its foot (`Move`, `Select`, `Change` on setting values vs `Select` on setting actions, `Resume`, `Back`), with small diamonds between the groups to echo the selection marks above, and the version tucked into the bottom-right corner on the title so the foot stays clear. Both the menu footer and the combat HUD strip switch on the fly between forged dark keycaps and icons for `Xbox` (weathered jewel rims), `PlayStation` (Cross, Circle, Square, Triangle and `L2`/`R2`/`R1`), `Switch` (bone-rimmed `B`/`A`/`Y`/`X` in Nintendo's swapped positions and `ZL`/`ZR`/`R`) or `Generic` (monochrome `A`/`B`/`X`/`Y` caps and a `Start` pill), classified from `GetGamepadName` on the active slot.

**The pause.** `PauseFight` holds the fight: `StepSession` does not step the world, and `Game` stops the particles, the slow-down and the shake, so the fight resumes exactly as it was. When the pause begins, the game draws the fight once into a texture at the game's resolution, then halves it three times with smooth filtering, down to 160x90. Behind the menu it draws that small copy stretched over the whole screen, darkened, and a second copy over it that shrinks to 94% over a quarter of a second and is darkened less. The fight seems to step back and go out of focus behind the menu.

**In a browser.** Esc is taken by the browser to free a locked pointer and never reaches the game, so the page tells the game when the pointer is let go mid-fight, and the game pauses. In the menus the game asks the page not to lock the pointer, so the mouse can point at the choices; the click that resumes a fight locks it again when the button comes up.

## Design decisions

- **Words, not boxes.** The games this one borrows from use a few words on the dark, and the fight's look is already built on Cinzel and the bone and grey of the HUD. A box around each button would be the busiest thing on the screen.
  - Rejected: framed buttons. Easier to read as buttons, but heavier than the rest of the HUD, and the marks already say which item is selected.
- **The menu logic is in `horde_core`.** Moving, wrapping, the mouse against the keys and going back are rules, so they are tested without a window. Only reading the devices and drawing live in the app.
- **A mouse that has not moved does not take the selection.** Otherwise the item under a resting mouse would keep winning against the keys every frame.
- **A blur without a shader.** Halving the picture three times with smooth filtering and stretching it back is a blur that works the same on the desktop and in WebGL, and costs four draws once per pause.
  - Rejected: a blur shader. Better looking, but one more shader to write for desktop OpenGL and for WebGL, for a picture that does not move.
- **Going back to the title starts the attempts over.** Returning to the title gives up the run of tries, as it does in the games this one borrows from; a retry keeps counting.
- **Left and right adjust, and a row turns them into moves.** In the settings, up and down go between lines and left and right change a value, as in most games. The menu does not know about values, so `StepMenu` only reports which way, and `Game` changes the setting.
- **The title is a state of the `Session`, not a separate screen object.** The world waits behind it, and beginning the fight is a change of state, so all the screens are one state machine with tests.
- **Unplugging the active gamepad pauses the fight, while plugging one in never interrupts it.** A dead battery or a pulled cable mid-fight would otherwise cost the attempt, whereas plugging a controller in works straight away on whichever of the four slots the system assigns it.
- **Prompts drawn in code, classified by name in `horde_core`.** Everything else in the game is procedural in code, so drawing the keycaps, mouse icons, PlayStation shapes, Xbox jewel rims, Switch letters and generic caps with raylib primitives keeps one dark-iron look and adds nothing to download in the web build. `ClassifyGamepadName` checks explicit brand and USB vendor tokens before bare HID names such as Sony's `"Wireless Controller"` on Windows DirectInput or Nintendo's `"Pro Controller"`, so `"Xbox Wireless Controller"` still lands on `Xbox` and unknown controllers fall back cleanly to `Generic`.
  - Rejected: an external prompt sprite pack. Crisper for tiny logos, but it would be the only image asset in an all-code game and would add weight to the web build.

## How to tune it

| What | Where | Now |
|---|---|---|
| The items of each screen | `kTitleItems`, `kPauseItems`, `kDeadItems`, `kVictoryItems`, `kSettingsRows`, `kControlsItems` (`MenuDraw.cpp`) | |
| Where the title and the menus sit | `kTitleY`, `kTitleMenuY`, `kPauseY`, `kPauseMenuY`, `kEndMenuY`, `kMenuSpacing` (`MenuDraw.cpp`) | The title block in the middle of the screen |
| The marks | `kMarkGap`, `kMarkLine`, `kMarkDiamond`, `kMarkSlideSeconds` (`MenuDraw.cpp`) | 14, 28, 4 pixels; 0.12 s |
| How far the mouse can miss an item | `kHitPadX`, `kHitPadY` (`MenuDraw.cpp`) | 18, 10 pixels |
| The pause's blur and step back | `Backdrop` halves, `kRecededScale`, `kRecedeSeconds`, `kEdgeDark`, `kMiddleDark` (`Backdrop.cpp`) | Down to 160x90; 94% over 0.25 s; 86% and 50% dark |
| The menu colors | `kMenuIdle`, `kMenuFaint` (`Palette.h`) | |
| Stick and d-pad repeat in menus | `MenuRepeatStats` (`MenuRepeat.h`) | Engage 0.5, release 0.35, 0.38 s delay, 0.11 s repeat |
| Footer and HUD button prompts | `InputPrompts.cpp`, `Palette.h` | 20 px face caps, 18 px keycaps, weathered Xbox/PlayStation rims |

## Tests

- `tests/core/MenuTests.cpp`: a new menu selects its first item; moving goes item by item and wraps at both ends; the mouse selects the item it moves onto, over a key in the same frame, and an item that does not exist is ignored; confirming chooses the selected item, and a click the item under the mouse; an adjust asks to change the selected item's value, on the item under the mouse, and a confirm in the same frame wins; going back wins over everything; an empty menu ignores everything.
- `tests/core/MenuRepeatTests.cpp`: immediate step on tilt, initial delay and steady repeat, hysteresis against thumb tremor vs immediate re-arm on release, dominant-axis selection on diagonals, latching a held stick on menu open, and clamping long frames.
- `tests/core/GamepadConnectionTests.cpp`: recording pads at start-up without hotplug events, plugging in and unplugging mid-run across slots, switching to the slot with input, and ignoring an idle slot unplugging while another slot is active.
- `tests/core/GamepadTypeTests.cpp`: classifying Xbox, PlayStation (including Windows DirectInput `"Wireless Controller"`), Nintendo Switch (including `"Pro Controller"`) and unrecognized controller names, plus positional button and trigger labels across all four families.
- `tests/core/SessionTests.cpp`: the title waits over a fresh world that does not move until the fight begins; a paused fight holds still, cannot restart, and resumes where it stopped; only a fight in progress pauses; going back to the title gives a fresh world and starts the attempts over.

The screens I checked with window captures in Release: the title with each item selected, a fight, the pause over the blurred fight, YOU DIED with its choices dim and then lit, D moving the selection to Return to title, and Enter taking the game back to the title. In the browser build, with keys and a click held across a frame (my test browser's own clicks are shorter than a frame), Enter began the fight, Esc paused it over the blurred still, and a click on Return to title went back to the title. Not checked: the pause when a real browser lets go of a locked pointer, which needs a real click to lock it first, and the gamepad's buttons.

## Limitations

- **The title's world is still.** The boss does not breathe behind the title, because the world does not step there.
