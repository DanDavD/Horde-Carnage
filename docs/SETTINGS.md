# Settings

## What it is

The few things a player may want to change before or during a fight: how loud the game is and how loud its music, how much the screen shakes, fullscreen or a window, and how often the game draws a frame. There is also a page that lists the controls. Settings opens from the title and from the pause menu, and every change takes effect at once and is saved for the next run.

The code:

- `src/core/Settings.h`, `src/core/Settings.cpp`: `Settings`, `AdjustSetting` (one step of one value), what each value means to the game (`MasterGain`, `MusicGain`, `ShakeAmount`, `TargetFramesPerSecond`), how it reads on screen (`SettingValueText`), and the saved text (`FormatSettings`, `ParseSettings`). Pure logic, with tests.
- `src/app/SettingsStore.h`, `src/app/SettingsStore.cpp`: where the text is kept. A file on the desktop, the page's local storage in a browser.
- `src/app/render/MenuDraw.cpp`: the settings and controls pages (`DrawSettingsScreen`, `DrawControlsScreen`) and where the mouse hits them (`MenuTargetAt`).
- `src/app/Game.cpp`: `ChangeSetting`, `ApplySettings` and `ApplyDisplay`, and the pages opened over the title and the pause (`OpenPage`, `GoBack`).

## How it works

**The values.**

| Line | Values | Default | What it changes |
|---|---|---|---|
| Master volume | 0 to 10, as ten marks | 10 | raylib's master volume: 0.8 x (level / 10) squared |
| Music volume | 0 to 10, as ten marks | 10 | The music's stream alone, under the master: (level / 10) squared. See [Music](MUSIC.md) |
| Screen shake | Off, Low, Full | Full | The shake drawn: none, half, all of it |
| Display | Fullscreen, Windowed | Fullscreen | A borderless window over the monitor, or a 1280x720 window that can be resized |
| Frame rate | VSync, 60, 120, 144, Unlimited | VSync | How often a frame is drawn. The fight always steps 60 times a second |
| Controls | | | Opens the table of controls |
| Back | | | Back to the title or the pause |

In a browser there is no Display or Frame rate line: the page decides the size of the game, and a browser draws once per refresh of the screen whatever the game asks.

**Changing a value.** W and S (or up and down) move between lines. A and D (or left and right, or the d-pad) change the selected value, and repeat while held, so the volume runs from one end to the other without tapping. Enter, Space or a click on a line raise its value by one; a click on the left arrow lowers it. The volumes stop at 0 and 10; the other values wrap round. The arrows show on each side of the selected value only.

**Taking effect.** `ChangeSetting` changes the value with `AdjustSetting`, then puts it into effect: the volume through `SetMasterVolume`, the music volume through the music's own stream (`SetMusicPlayerVolume`), the frame rate by turning vsync on or off and setting raylib's target, the display by toggling the borderless window. The shake is read when the world is drawn, so it needs nothing. A tick plays after every change, at the new volume, which is how the volume is set by ear.

**Saving.** Every change writes all the settings as five lines of text:

```
volume=8
music=6
shake=low
display=fullscreen
framerate=vsync
```

On the desktop that is `settings.txt` next to the game's executable; in a browser it is the page's local storage, under `horde-carnage-settings`. The game reads it before the window opens, so a game saved as windowed never flashes fullscreen first. `--windowed` on the command line still opens a window for that run, and changing another setting during that run saves the display that was saved before, not the window.

**The controls page.** A table of what each key and button does, in three columns: the action, the keyboard and mouse, and the gamepad. Back returns to Settings, on its Controls line.

## Design decisions

- **A handful of values, not a full options menu.** These are the settings the players of a short action game look for first, and each one fixes a real problem: no sound control, motion sickness from shake, a game that will not leave fullscreen, and a frame rate that heats a laptop.
- **The music has a volume of its own.** A player who wants to hear the warnings over everything, or to play to their own music, turns the theme down without losing the effects. It acts on the music's stream, under the master volume, so turning the master down still turns everything down. A file saved before this line existed has no `music=` and reads with the music at full.
  - Rejected: an effects volume as well. The effects carry the warnings of every attack, which a player should not turn off by accident; the master volume is enough for them.
  - Rejected: remapping keys. It needs a page that waits for a key, a check for clashes and a reset, about a day of work for a fight with six actions. The controls page shows them instead.
- **The rules are in `horde_core`.** Stepping a value, the volume curve and reading a damaged file are the parts that can go wrong quietly, so they are tested without a window. Only where the text is kept and what raylib is told live in the app.
- **The volume is squared.** The ear hears loudness roughly on a log scale. With a straight line, the top half of the marks would sound almost the same and all the change would sit in the bottom two. Full volume is still 0.8, the headroom the fight's loudest moments need.
- **The volume stops at its ends; the choices wrap round.** Holding left to turn the game down must never jump it to full. A list of three or five choices, on the other hand, is quicker to go round than to go back through.
- **A text file of `key=value` lines.** It is easy to read and to fix by hand, needs no library, and an unknown line from a later version is skipped. A value that cannot be read keeps its default, and a volume out of range is brought back into it, so a damaged file never stops the game.
  - Rejected: JSON. It would need a dependency for four values.
- **Next to the executable.** The game is a zip a player unpacks anywhere, so its folder is where they would look for its files, and deleting the folder removes everything.
  - Rejected: the user's AppData folder. It is the Windows convention for installed programs, but it leaves a file behind that the player never sees.
- **Saved on every change, not on leaving the page.** The file is a few bytes, and a game closed from the settings page keeps what was set.
- **The borderless window is not kept on top.** raylib's borderless mode stays above every other window, so after switching to another program the game kept covering it. A window the size of the monitor already hides the taskbar without it.
- **Settings opens over the title and the pause, without a state of its own in `Session`.** It does not change the fight, only how it is shown, so the session stays in `Title` or `Paused` and `Game` keeps which page is open.

## How to tune it

| What | Where | Now |
|---|---|---|
| The defaults | `Settings` (`Settings.h`) | 10, 10, Full, Fullscreen, VSync |
| The steps of the volume | `kMaxVolume` (`Settings.h`) | 10 |
| The loudness at full volume | `kFullGain` (`Settings.cpp`) | 0.8 |
| How much Low shakes | `ShakeAmount` (`Settings.cpp`) | Half |
| The frame rates offered | `FrameRateCap`, `TargetFramesPerSecond` (`Settings.h`, `Settings.cpp`) | VSync, 60, 120, 144, Unlimited |
| The lines of the page | `kSettingsRows` (`MenuDraw.cpp`) | |
| Where the lines sit | `kRowFirstY`, `kRowSpacing`, `kRowGap`, `kRowHalf`, `kValueCenterX`, `kArrowOffset` (`MenuDraw.cpp`) | |
| The controls table | `kControlsLines` (`MenuDraw.cpp`) | |
| The file and the storage key | `SettingsPath`, `ReadStoredSettings` (`SettingsStore.cpp`) | `settings.txt`, `horde-carnage-settings` |

## Tests

- `tests/core/SettingsTests.cpp`: the defaults; the volume stops at 0 and 10 while the other values wrap round both ways; the volume curve is silent at 0, under 1 at 10, and at 5 a quarter of full; the music volume stops at both ends, is squared to 1 at full and leaves the master alone; a file without a music line reads with the music at full; the shake and frame rate turn into the right numbers; each value reads in capitals; settings written as text read back the same; and a damaged file (Windows line ends, spaces, an unknown key, an unknown value, a line without `=`, a number with junk after it, a volume out of range) keeps the defaults for what it gets wrong.
- `tests/core/MenuTests.cpp`: an adjust asks to change the selected item's value, on the item the mouse points at, and a confirm in the same frame wins.

What I checked in a Release build, with keys posted to a window resized to 1600x900:

- The title with Settings, the settings page, two presses of A taking the volume to 8, Screen shake set to Low, Frame rate to 60, the controls page, and Esc twice going back to Settings on Controls and then to the title on Settings. The file then read `volume=8`, `shake=low`, `display=fullscreen`, `framerate=60`: the run used `--windowed`, and the saved display stayed fullscreen.
- Settings over the pause, over the blurred fight.
- The frame rate in the HUD's counter: 60 with 60, 118 after changing to 120 mid-fight, 1676 with Unlimited, 164 with VSync on my 165 Hz monitor.
- Display, changed on the settings page: from a window to 1920x1080 and back to 1280x720. Before I took the borderless window off the top, it minimized itself in three of my first runs, always while I was using another program; after, it did not in five runs, and it was not kept on top. A game saved as windowed started in a window.

For the music volume, in a Debug build: three presses of A on its line lit seven of its marks and the file read `music=7`. In the browser build, with the music volume taken to 0, a fight began in silence (the page's audio output measured nothing, against -25 dB at full), and the page's storage held `music=0`.

In the browser build, the settings page showed only the volumes and the shake. After changing both, the page's storage held the new text, and after reloading the page the settings page showed them again.

## Limitations

- **The keys cannot be changed.** See the design decisions.
- **Display needs a real screen to judge.** Window captures of a fullscreen game are stale, so I checked the window's size and state, not its picture. I never found what minimized the window in those first runs, only that it stopped once the window was no longer kept on top, so it is worth watching for.
- **A folder the game cannot write to.** If the game sits where it may not write, such as Program Files, the settings last for that run only. raylib logs a warning and nothing else happens.
- **The controls table adapts dynamically.** When a gamepad is connected, the Gamepad column adapts to the detected controller family (Xbox, PlayStation, Switch or generic); with keyboard and mouse it displays multi-platform notation.
