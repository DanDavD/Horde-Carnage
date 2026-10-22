# Game flow

## What it is

What happens around a run: playing, dying, and starting again. It is a small state machine with two states, `Playing` and `GameOver`. Day 12 adds the main menu and pause as more states.

The rules live in `horde_core`:

- `src/core/Session.h`, `src/core/Session.cpp`: the `Session` (the `World` plus the state machine), `StartSession`, `StepSession`, `CanRestart` and `TryRestart`.

The game executable connects it to the screen and the buttons:

- `src/app/Game.cpp`: `Game` owns one `Session`. `Run` reads the restart buttons once per frame, `Update` calls `StepSession` every fixed step, and `Draw` shows the game over screen.
- `src/app/PlayerInput.cpp`: `ReadRestartPressed` reads Enter or R, and A or Start on the first gamepad.

## How it works

```
            player's health reaches 0
  Playing  ---------------------------->  GameOver
     ^                                        |
     |   TryRestart, after 1 s in GameOver    |
     +----------------------------------------+
```

- **Playing:** `StepSession` calls `StepWorld` every fixed step. If the player's health is 0 after the step, the session enters `GameOver`.
- **GameOver:** the world is not stepped any more. It stays frozen on the moment of death, so the player can see what got them. The screen darkens and shows "GAME OVER", how long the player survived (`World::elapsedSeconds`), and how to restart.
- **Restart:** `TryRestart` works only in `GameOver`, and only after `kRestartDelaySeconds` (1 s). It replaces the world with a brand new one from `CreateWorld` and goes back to `Playing`. Until the delay has passed, the restart prompt is dimmed.

Every state change goes through one helper, `EnterState`, which also resets the clock of the state (`stateSeconds`). That clock is what the restart delay reads.

## Design decisions

- **The state machine lives in `horde_core`, not in `Game`.** Dying, freezing and restarting are rules, and rules belong where the tests can reach them. The tests play a whole run until game over and restart it, without a window.
- **An `enum class` and a `switch`, not a class per state.** With two states and one transition each, a class hierarchy would be more code than behaviour. When the menu and pause arrive on day 12 I will look at it again. The enemy and boss state machines (days 7 and 11) have more states with their own data, and those are a better fit for one type per state.
- **Restart is a brand new `World`, not a reset.** Resetting fields one by one means every new field needs a matching line in the reset, and the one that gets forgotten leaks from one run into the next. `CreateWorld` is already the single place that knows what a fresh world looks like.
- **A one second delay before restart is accepted.** Someone mashing buttons in a fight would otherwise skip the game over screen before reading it. Attack buttons arrive on day 4, and A on a gamepad will likely be one of them.
- **Restart buttons are read once per frame, not once per fixed step.** A press is an event. A frame that runs no fixed step (possible above 60 FPS) would lose it if it were read inside the step. Movement keys are held states, so reading them in the step is fine.
- **A frozen world is drawn without interpolation.** Drawing blends the last two steps by `alpha`. When the world stops stepping, those two steps stay fixed while `alpha` keeps changing every frame, so everything would shake between them. In `GameOver`, `Draw` uses `alpha = 1`, the last position.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Delay before restart is accepted | `kRestartDelaySeconds` (`src/core/Session.h`) | 1 s |
| Restart buttons | `ReadRestartPressed` (`src/app/PlayerInput.cpp`) | Enter, R; A, Start |

## Tests

- `tests/core/SessionTests.cpp`: a new session is playing with full health; while playing, a session step is a world step; the game is over in the step the health reaches 0; once over, the world is frozen (player, enemies and survival time do not change); restart is refused while playing and inside the delay; after the delay a restart gives a fresh world (full health, time 0, everyone back at the start); and standing still among the hairballs ends in a game over.

The game over screen and the restart buttons are not unit tested, because they need a window. I checked them in a Release build: standing still until the game over screen, then pressing Enter.

## Limitations

- **There is no title screen.** The game starts playing as soon as the window opens. The main menu comes on day 12.
- **There is no best time.** The game over screen shows only the last run, and nothing is saved between launches.
- **Esc still quits at once, even in the middle of a run.** Pause comes on day 12.
