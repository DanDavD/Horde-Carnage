# The web build

## What it is

The same game, compiled to WebAssembly with Emscripten so it runs in a browser tab. It is the build I publish on itch.io: whoever looks at my work can play it with one click, without downloading or installing anything. The Windows zip on GitHub Releases stays for anyone who prefers it.

Nothing in the game's rules changes for the web. `horde_core` compiles as it is; only the main loop and the build files know about the browser.

The pieces:

- `CMakePresets.json`: the `web` configure and build presets (Emscripten's toolchain, Ninja, Release, no tests).
- `cmake/Dependencies.cmake`: with the Emscripten toolchain, raylib is built for its `Web` platform (WebGL, the browser's input and audio).
- `src/CMakeLists.txt`: the web target writes `index.html`, `index.js` and `index.wasm`, links GLFW from Emscripten's own port, and lets the memory grow.
- `web/shell.html`: the page around the game.
- `src/app/Game.cpp`: `Game::Run` hands frames to the browser instead of looping.
- `.github/workflows/ci.yml`: a `web` job builds it on every push and pull request and keeps the page as an artifact.

## How it works

**The main loop.** On the desktop, `Game::Run` loops until the window closes. A browser tab cannot be held in a loop: the page would freeze. So the body of the loop is `Game::Frame`, and in a browser `Run` gives it to `emscripten_set_main_loop_arg`, which calls it once per browser frame (`requestAnimationFrame`). The fixed timestep does the rest, exactly as on the desktop: however often the browser draws, the simulation steps at 60 Hz. With its last argument set, the call never returns, so the `Game` on `main`'s stack lives as long as the page.

**The page.** `web/shell.html` is a dark page with the game in the middle and a "Loading..." line until it starts. The canvas covers the whole page and raylib resizes it with the page, so going fullscreen on itch.io fills the screen; the game draws at the canvas's resolution and adds its own bars (see [Screen and fullscreen](SCREEN.md)). The page also keeps Space and the arrow keys from scrolling it.

**The aim.** In a browser, raylib 5.5 stops following the mouse as soon as the cursor is hidden, which the game does while playing to draw its own crosshair: the shots still fired, but the aim froze. So the page tracks the aim point itself, in `Module.hordeAim`, in canvas pixels (it divides by any scale the browser applies), and `ReadAimPoint` (`src/app/PlayerInput.cpp`) reads it every frame. A click on the game gives it the keyboard and locks the pointer to it, as browser shooters do, so the mouse cannot wander out of the game in the middle of a fight; while locked, the mouse only reports how far it moved, and the page moves the aim point by that, inside the game. Esc gives the pointer back, and the next click locks it again. On the desktop, `ReadAimPoint` is raylib's mouse position, as before.

**What the player sees differently.** The controls line leaves out "Esc quit": in a browser the tab is closed, not the game, and Esc gives the mouse back. The system cursor is hidden by the pointer lock, not by the game (`SetCursorHidden` does nothing in a browser).

## Design decisions

- **Emscripten, through raylib's own web platform.** raylib supports the browser out of the box, so the port is build settings and one change to the main loop, not a second renderer.
  - Rejected: a separate web front end. Two renderers would drift apart, and the point of the project is one game.
- **`emscripten_set_main_loop_arg`, not ASYNCIFY.** ASYNCIFY would let the desktop `while` loop run unchanged, but it rewrites the whole program to pause and resume, which makes it bigger and slower. Splitting the loop body into `Frame` is a small change that keeps the desktop and the web on the same code.
- **The game fits itself to the canvas, the page does not scale it.** At first the page scaled a 1280x720 canvas with CSS, which blurred it on a big screen. Now the canvas has the page's real size and the game draws at that resolution, the same way it does on the desktop (see [Screen and fullscreen](SCREEN.md)).
- **Release only, without tests.** The tests run on the Windows job against the same `horde_core`; running them again under Node would add a slow step and test nothing new. The web job proves that the game builds for the browser.
- **A pinned Emscripten (3.1.74).** Like raylib and doctest, the compiler is pinned, so a new Emscripten release cannot break the build on its own.
- **The page tracks the aim, not raylib.** raylib's web platform treats a hidden cursor as a locked one and stops updating the mouse position. Rather than patch raylib, the page, which already knows the canvas's scale, keeps the aim point, and the game asks for it.
  - Rejected: showing the system cursor while playing. It fixes the aim, but the cursor and the crosshair would both be drawn, and the mouse could still leave the game.
- **The pointer is locked on a click.** A mouse that leaves the game while the player strafes with the mouse button held is lost aim, or a click on another tab. Browsers only lock the pointer after a click, which the first shot gives.
- **One warning off for Clang.** Clang's `-Wextra` flags every designated initializer that names only some fields, which is how the code sets its numbers (`MovementStats{.maxSpeed = 120.0f}`). `-Wno-missing-field-initializers` turns that one off for GCC and Clang (`cmake/Warnings.cmake`); every other warning is still an error.

## How to build it

Once, install the Emscripten SDK (about 1 GB, outside the repository):

```
git clone https://github.com/emscripten-core/emsdk.git C:\emsdk
C:\emsdk\emsdk.bat install 3.1.74
C:\emsdk\emsdk.bat activate 3.1.74
```

Then, in each new terminal, activate it and build (PowerShell):

```
. C:\emsdk\emsdk_env.ps1
cmake --preset web
cmake --build --preset web
```

The page is `build/web/src/index.html`, with `index.js` and `index.wasm` next to it. It does not open from the file system (browsers refuse to load WebAssembly from `file://`), so serve the folder:

```
python -m http.server 8085 --directory build/web/src
```

and open `http://localhost:8085`.

## How to publish it on itch.io

1. Zip `index.html`, `index.js` and `index.wasm` (the three files at the top of the zip, not in a folder).
2. On itch.io: *Create new project*, kind of project *HTML*, upload the zip and tick *This file will be played in the browser*.
3. Embed options: viewport 1280x720, *Fullscreen button* on, and *Click to launch in fullscreen* off.
4. Save, check the page, and set it to public.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Emscripten version | `.github/workflows/ci.yml` and the install commands above | 3.1.74 |
| Link options | `target_link_options` in `src/CMakeLists.txt` | `-sUSE_GLFW=3 -sALLOW_MEMORY_GROWTH=1` |
| The page around the game | `web/shell.html` | Dark, the canvas over the whole page |

## Tests

There are no web-only tests: the rules are the same `horde_core` the Windows job tests (278 cases). I checked the browser build by hand in a local server, in a window smaller than the game: the whole arena scaled into it; eight pistol clicks took the Shell to about half; standing still ended in "YOU DIED", "Slain by her Leap after 0:07"; and Enter started attempt 2. My first check of the aim was wrong: I had placed the mouse before the restart and not moved it after, so the frozen crosshair happened to sit under it. On itch.io the aim did not follow the mouse. After the fix, during a fight, I moved the mouse to three points in turn and the crosshair and the gun followed each one. The pointer lock needs a real click in a real browser, which my test browser cannot give, so it is checked on the itch.io draft page. The web job in CI only checks that it builds.

## Limitations

- **raylib's own mouse position is not used in the browser.** Anything new that reads the mouse there has to go through `ReadAimPoint`.
- **Very short key presses can be missed.** raylib reads a key as pressed when it is down at the start of a frame. A real press lasts several frames, but a key sent and released inside one frame (as some automation does) does not count. The same is true on the desktop.
- **No gamepad testing in the browser.** The browser exposes gamepads, and raylib reads them, but I have not tried one.
- **Sound will need a click first.** Browsers only start audio after the player interacts with the page. The title screen, when it comes, gives the player that click.
