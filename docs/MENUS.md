# Menus

## What it is

The screens around the fight: the title, the pause menu, and the choices on the end screens. They share one look, taken from the games this one borrows from: a few words in capitals on the dark, grey when they are not selected, and the selected one in bone between a thin line and a small diamond on each side, in the manner of Dark Souls and Hollow Knight. There are no boxes and no buttons to draw, only the words. Color is kept for what means something: the red of "YOU DIED" and the gold of "VICTORY".

The code:

- `src/core/Menu.h`, `src/core/Menu.cpp`: `Menu`, `MenuInput` and `StepMenu`, the selection and what one frame of input does to it.
- `src/core/Session.h`, `src/core/Session.cpp`: the `Title` and `Paused` states, `ReturnToTitle`, `BeginFight`, `PauseFight` and `ResumeFight` (see [Game flow](GAME_FLOW.md)).
- `src/app/render/MenuDraw.h`, `src/app/render/MenuDraw.cpp`: the items of each screen, where they sit, which one the mouse is over, and how they are drawn; the title and the pause screens.
- `src/app/render/Backdrop.h`, `src/app/render/Backdrop.cpp`: the blurred still of the fight behind the pause menu.
- `src/app/PlayerInput.cpp`: `ReadMenuKeys`, `ReadPausePressed`, `ReadQuickRetryPressed` and, in a browser, `TakePointerLockLost`.
- `src/app/Game.cpp`: `HandleMenus` and `Choose`, which turn a choice into a change of state.

## How it works

**The screens.**

| Screen | Items | Back (Esc, B) |
|---|---|---|
| Title | Begin, Quit (no Quit in a browser) | Nothing |
| Pause | Resume, Return to title | Resume |
| YOU DIED | Try again, Return to title | Nothing |
| VICTORY | Fight again, Return to title | Nothing |

The game opens on the title, over a fresh world that waits behind it, darkened. The game's name and its menu sit together in the middle of the screen. Esc (or Start on a gamepad) pauses a fight. The end screens' choices sit in a row at the foot of the dark band, and stay dim until the restart delay has passed. R still retries at once after a death or a victory, for the next attempt without the menu.

**Choosing.** `StepMenu` takes one frame of input from anything: W, S and the arrows (or the d-pad) move, and up and left both mean the previous item, so a row and a column read the same. Enter or Space (or A) confirm, and Esc or Backspace (or B) go back. The mouse selects the item it moves onto and a click chooses the item under it. A mouse that has not moved does not take the selection, so a player using the keys is not pulled back to wherever the mouse rests. The selection wraps round at both ends. Going back wins over everything else in the same frame. Each move plays a dry tick and each choice a low knock, both synthesized like the rest of the sounds (see [Audio](AUDIO.md)).

**The marks.** When the selection moves, the two lines beside the new item slide out over 0.12 s from its diamonds. It is the only animation in the menus.

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
- **The title is a state of the `Session`, not a separate screen object.** The world waits behind it, and beginning the fight is a change of state, so all the screens are one state machine with tests.

## How to tune it

| What | Where | Now |
|---|---|---|
| The items of each screen | `kTitleItems`, `kPauseItems`, `kDeadItems`, `kVictoryItems` (`MenuDraw.cpp`) | |
| Where the title and the menus sit | `kTitleY`, `kTitleMenuY`, `kPauseY`, `kPauseMenuY`, `kEndMenuY`, `kMenuSpacing` (`MenuDraw.cpp`) | The title block in the middle of the screen |
| The marks | `kMarkGap`, `kMarkLine`, `kMarkDiamond`, `kMarkSlideSeconds` (`MenuDraw.cpp`) | 14, 28, 4 pixels; 0.12 s |
| How far the mouse can miss an item | `kHitPadX`, `kHitPadY` (`MenuDraw.cpp`) | 18, 10 pixels |
| The pause's blur and step back | `Backdrop` halves, `kRecededScale`, `kRecedeSeconds`, `kEdgeDark`, `kMiddleDark` (`Backdrop.cpp`) | Down to 160x90; 94% over 0.25 s; 86% and 50% dark |
| The menu colors | `kMenuIdle`, `kMenuFaint` (`Palette.h`) | |

## Tests

- `tests/core/MenuTests.cpp`: a new menu selects its first item; moving goes item by item and wraps at both ends; the mouse selects the item it moves onto, over a key in the same frame, and an item that does not exist is ignored; confirming chooses the selected item, and a click the item under the mouse; going back wins over everything; an empty menu ignores everything.
- `tests/core/SessionTests.cpp`: the title waits over a fresh world that does not move until the fight begins; a paused fight holds still, cannot restart, and resumes where it stopped; only a fight in progress pauses; going back to the title gives a fresh world and starts the attempts over.

The screens I checked with window captures in Release: the title with each item selected, a fight, the pause over the blurred fight, YOU DIED with its choices dim and then lit, D moving the selection to Return to title, and Enter taking the game back to the title. In the browser build, with keys and a click held across a frame (my test browser's own clicks are shorter than a frame), Enter began the fight, Esc paused it over the blurred still, and a click on Return to title went back to the title. Not checked: the pause when a real browser lets go of a locked pointer, which needs a real click to lock it first, and the gamepad's buttons.

## Limitations

- **No settings yet.** They come in the next commit, with their own screen.
- **The gamepad's stick does not move the selection.** The d-pad does; the stick comes with the gamepad support.
- **The title's world is still.** The boss does not breathe behind the title, because the world does not step there.
