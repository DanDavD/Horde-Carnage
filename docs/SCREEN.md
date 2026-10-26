# Screen and fullscreen

## What it is

How the game fills the screen. On the desktop it always runs fullscreen, at the monitor's own resolution and refresh rate; in a browser it fills the page, and the whole screen when itch.io goes fullscreen. Whatever the screen, the game is laid out for one resolution, 1280x720, and drawn at the screen's real resolution, scaled as a whole and centred, with black bars where the screen's shape differs. Every player sees the same part of the arena.

The pieces:

- `src/core/ScreenFit.h`, `src/core/ScreenFit.cpp`: `FitScreen` works out the scale and the offset that place the game on a screen, and `ScreenToGame` and `GameToScreen` convert points between the two. Pure math, with tests.
- `src/app/Game.cpp`: the window (`Game::Game`), and the bars and the two cameras around each frame (`Game::Draw`).
- `src/app/render/View.cpp`: the fit of this frame (`CurrentScreenFit`), the two cameras (`WorldCamera`, `ScreenCamera`), and the mouse in game pixels (`AimInGame`).
- `src/main.cpp`: the `--windowed` option.
- `web/shell.html`: in a browser, the canvas follows the page's size.

## How it works

**The window.** On the desktop, `Game::Game` opens the window and makes it a borderless window over the whole monitor (`ToggleBorderlessWindowed`), at the monitor's resolution. With vsync on, it draws at the monitor's refresh rate: on a 165 Hz monitor, 165 frames a second. The simulation still steps at 60 Hz and the drawing interpolates between steps (see [Architecture](ARCHITECTURE.md)), so the game runs the same on every monitor. `--windowed` on the command line opens a 1280x720 window that can be resized instead.

In a browser, the window is the canvas, and raylib resizes it with the page (`FLAG_WINDOW_RESIZABLE`). `SetWindowMinSize` right after opening makes raylib fit it to the page once at the start. When itch.io goes fullscreen, the page is the whole screen, and so is the canvas.

**Fitting the game.** Every frame, `CurrentScreenFit` asks `FitScreen` for the largest scale at which 1280x720 fits the screen whole: 1.5 on 1920x1080, 2 on 2560x1440, 3 on 3840x2160. On a screen of another shape, one side decides, and the other is split between two bars. A 3440x1440 ultrawide gets a scale of 2 and two bars 440 pixels wide.

**Drawing.** `Game::Draw` clears the screen to black, which is the bars, and clips everything else to the game's area (`BeginScissorMode`). Then:

- the world is drawn through the follow camera, with its zoom set to the scale and its centre moved to the middle of the game's area on the screen;
- the HUD, the end screens and the crosshair are drawn through a second camera, `ScreenCamera`, which maps game pixels to the screen with the same scale and offset.

Both are drawn at the screen's real resolution. Since everything is shapes and text, not images, nothing is stretched: a circle drawn at 1920x1080 is as sharp as the screen allows. The default font is a small bitmap, so it is smoothed (`TEXTURE_FILTER_BILINEAR`) to scale evenly.

**The mouse.** `ReadAimPoint` gives the mouse in screen pixels. `AimInGame` turns it into game pixels with `ScreenToGame` and keeps it inside the game, so a mouse over a bar aims at the edge. The world point the player aims at comes from that, through the camera at 1280x720, exactly as before.

## Design decisions

- **One resolution for the game, scaled to the screen.** The camera, the HUD and every distance the player sees are laid out once. On a bigger screen the picture is bigger, not wider.
  - Rejected: showing more of the arena on a wider screen. It would make the fight easier on an ultrawide: a Charge from off screen is seen sooner, and a Summon's marks are seen from further away.
- **Drawn at the screen's resolution, not drawn small and stretched.** Drawing at 1280x720 into a texture and stretching it to the screen is simpler, but blurs every edge at 1.5x. Drawing through a scaled camera costs nothing more and stays sharp.
- **Bars, not a stretched picture.** Stretching a 16:9 picture onto 16:10 or 21:9 squashes circles into ovals, and the boss's warnings are shapes the player reads.
- **A borderless window, not exclusive fullscreen.** Exclusive fullscreen changes the monitor's mode, makes switching to another program slow and can leave the desktop at the wrong resolution after a crash. A borderless window over the monitor looks the same and has none of that.
- **Always fullscreen, with `--windowed` for development.** The game is meant to be played fullscreen, so that is the default and there is no key to leave it. A window is still useful to record the game, to debug it next to an editor, or to capture it with tools that cannot see a fullscreen game.
- **The fit is pure math in `horde_core`.** It is the kind of code that is easy to get subtly wrong (an off-centre bar, a mouse that drifts on one side of an ultrawide), and as a function it can be tested on screens I do not have.

## How to tune it

| Value | Where | Default |
|---|---|---|
| The game's resolution | `kScreenWidth`, `kScreenHeight` (`src/app/render/View.h`) | 1280x720 |
| Fullscreen or windowed | `--windowed` on the command line | Fullscreen |
| Colour of the bars | `ClearBackground` in `Game::Draw` | Black |
| How the default font scales | `SetTextureFilter` in `Game::Game` | Bilinear |

## Tests

- `tests/core/ScreenFitTests.cpp`: a 16:9 screen fills with the game, at 1.5 on 1920x1080 and 3 on 3840x2160; a wider screen gets bars on the sides and a taller one above and below; screen points and game points convert both ways, and a point on a bar lands outside the game; and a screen with no area gives the identity fit.

The window and the drawing need a real screen. On my 1920x1080, 165 Hz monitor, a Debug build logged its size and frame rate once a second: 1920x1080, 165 frames a second, the fight's clock advancing. Window captures of a fullscreen game show only its first frame, because it presents straight to the monitor, so the pictures come from `--windowed`: a window resized to 1500x700 showed the game scaled with bars on both sides, and the gun turned to the crosshair under the mouse. In a browser 638x347 pixels in size, the canvas matched the page, and the crosshair sat on the mouse at two points.

## Limitations

- **There is no key to leave fullscreen.** Esc still quits at once. A pause menu, where Esc will lead, comes with the menus.
- **The default font looks soft when scaled.** It is a small bitmap font. The art pass brings a real font, loaded at the size it is drawn at.
- **In a browser the canvas ignores the screen's pixel density.** On a laptop with a high-density screen, the canvas has as many pixels as the page has CSS pixels, so the picture is a little softer than the screen could show.
- **Only the primary monitor.** The borderless window covers the monitor the window opens on.
