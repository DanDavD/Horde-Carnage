# Game flow

## What it is

What happens around a fight: playing, dying or winning, and starting again. It is a small state machine with three states, `Playing`, `Dead` and `Victory`, plus a count of attempts. Day 11 adds the main menu and pause as more states.

The rules live in `horde_core`:

- `src/core/Session.h`, `src/core/Session.cpp`: the `Session` (the `World`, the state machine and the attempt count), `StartSession`, `StepSession`, `CanRestart` and `TryRestart`.
- `src/core/World.h`, `src/core/World.cpp`: `IsBossDefeated`, which says when the fight is won; `shotsHit` and `Accuracy`, for the victory screen; `lastHitBy`, the `DamageSource` (`src/core/DamageSource.h`) of the last hit on the player, for the death screen; and `DefeatBoss`, for the Debug shortcut.

The game executable connects it to the screen and the buttons:

- `src/app/Game.cpp`: `Game` owns one `Session`. `Run` reads the restart buttons once per frame, `Update` calls `StepSession` every fixed step, and `Draw` shows the end screens (`DrawEndScreen`, `src/app/render/EndScreen.cpp`) and the attempt number (`DrawAttempt`, `src/app/render/Hud.cpp`).
- `src/app/PlayerInput.cpp`: `ReadRestartPressed` reads Enter or R, and A or Start on the first gamepad.

## How it works

```
                      the boss is defeated
          +---------------------------------------->  Victory
          |                                              |
       Playing  -------------------------------->  Dead  |
          ^      the player's health reaches 0      |    |
          |                                         |    |
          |  TryRestart after 1 s: attempt + 1      |    |
          +-----------------------------------------+    |
          |                                              |
          |  TryRestart after 1 s: attempt back to 1     |
          +----------------------------------------------+
```

- **Playing:** `StepSession` calls `StepWorld` every fixed step. After the step it checks the boss first: if `IsBossDefeated`, the session enters `Victory`. Otherwise, if the player's health is 0, it enters `Dead`.
- **Dead and Victory:** the world is not stepped any more. It stays frozen on the moment the fight ended, so the player can see what got them, or the boss lying on the floor. The fight's clock (`World::elapsedSeconds`) stops with it.
- **Restart:** `TryRestart` works in `Dead` and `Victory`, and only after `kRestartDelaySeconds` (1 s). It replaces the world with a brand new one from `CreateWorld` and goes back to `Playing`.
- **Attempts:** `Session::attempt` starts at 1. A retry after a death adds 1. A new fight after a victory starts again from 1. It lives in the `Session`, not in the `World`, so the fresh world of a restart does not reset it.
- **Seeds:** every fresh world gets the next seed from `Session::seeds`, so each attempt plays out differently, and the session's one seed decides them all (see [Random numbers](RANDOM.md)).

Every state change goes through one helper, `EnterState`, which also resets the clock of the state (`stateSeconds`). That clock is what the restart delay and the fade of the end screens read.

**The end screens.** The world dims and a dark band with soft edges crosses the middle of the screen. It carries one big word:

- **YOU DIED**, in blood red, with the attempt number, what killed the player and when ("Attempt 4. Slain by her Leap after 0:23.", or her Charge, her Hair Ring, her Volley, her shockwave, her frenzy, her touch), and how much of her health the boss kept, rounded up so a living boss never reads 0%.
- **VICTORY**, in gold, with the attempt that won, the length of the fight, and the shots on target out of the shots fired.

Everything fades in over the restart delay, and the prompt to try again (or to fight her again) lights up when a press starts working. The attempt number also stays in the top right corner while playing.

**Debug shortcuts.** In Debug builds `F3` starts the boss's transformation (`TransformBoss`), and `F4` calls `DefeatBoss`, so the next step ends the fight in `Victory` without emptying both bars.

## Design decisions

- **The state machine lives in `horde_core`, not in `Game`.** Dying, winning, freezing and restarting are rules, and rules belong where the tests can reach them. The tests play a whole fight until it ends and restart it, without a window.
- **An `enum class` and a `switch`, not a class per state.** With three states and one or two transitions each, a class hierarchy would be more code than behaviour. When the menu and pause arrive on day 11 I will look at it again. The boss's state machines (days 6 to 8) have more states with their own data, and those are a better fit for one type per state.
- **The session asks the world whether the boss is defeated, instead of reading the boss's health.** An empty bar in the first phase starts the transformation instead of ending the fight, so the two are not the same. When the second phase came, only `IsBossDefeated` changed: it is true only in that phase.
- **When the boss and the player fall in the same step, the fight is won.** The last bullet landed, so the player did beat the boss. In the current fight this needs a hairball, because a dead boss stops hurting in the same step it dies, but the summons of phase 2 make it possible.
  - Rejected: a death wins ties. Losing a fight on the frame you won it feels like a bug, not like difficulty.
- **The world freezes on victory too.** It keeps the fight's clock exact for the screen, and the dead boss stays where it fell.
  - Rejected: letting the player walk around after the win, as in Dark Souls. The clock would need a separate "fight time", and there is nothing to do in an empty arena yet.
- **The attempt count lives in the `Session`.** The `World` is thrown away on every restart, which is what keeps runs from leaking into each other, so anything that must survive a restart cannot be in it.
- **A victory starts the count again.** "Defeated on attempt 7" is the line worth showing. After that, the next fight is a new challenge, not attempt 8 of the old one.
- **Restart is a brand new `World`, not a reset.** Resetting fields one by one means every new field needs a matching line in the reset, and the one that gets forgotten leaks from one run into the next. `CreateWorld` is already the single place that knows what a fresh world looks like.
- **A one second delay before restart is accepted.** Someone mashing buttons in a fight would otherwise skip the end screen before reading it. The screen fades in over the same second, so the prompt lights up exactly when a press works.
- **Restart buttons are read once per frame, not once per fixed step.** A press is an event. A frame that runs no fixed step (possible above 60 FPS) would lose it if it were read inside the step. Movement keys are held states, so reading them in the step is fine.
- **A frozen world is drawn without interpolation.** Drawing blends the last two steps by `alpha`. When the world stops stepping, those two steps stay fixed while `alpha` keeps changing every frame, so everything would shake between them. In `Dead` and `Victory`, `Draw` uses `alpha = 1`, the last position.
- **The prompt sits inside the band.** Below it, it landed on top of the player, who stands near the middle of the screen.
- **The death screen names what killed the player.** The fight is learned by dying, and "Slain by her Charge" turns a death into a lesson: that is the attack to watch for next time. `HurtPlayer` is already the single door for damage, so it records the source of every hit that lands, and the last one is the one that killed.
  - Rejected: naming the attack the boss was doing at the moment of death. A strand from a ring can land while the boss has already started its next attack, and a touch can kill during any of them.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Delay before restart is accepted (and the fade-in) | `kRestartDelaySeconds` (`src/core/Session.h`) | 1 s |
| When the fight is won | `IsBossDefeated` (`src/core/World.cpp`) | the boss's health at 0 in its second phase |
| Restart buttons | `ReadRestartPressed` (`src/app/PlayerInput.cpp`) | Enter, R; A, Start |
| Band size and position | `kEndBandTop`, `kEndBandHeight`, `kEndBandFeather` (`src/app/render/EndScreen.cpp`) | 240, 220, 48 pixels |
| Colours of the two words | `kBlood`, `kVictoryGold` (`src/app/render/Palette.h`) | |
| What the death screen calls each killer | `DeathCause` (`src/app/render/EndScreen.cpp`) | "Slain by her Charge" and so on |

## Tests

- `tests/core/SessionTests.cpp`:
  - a new session is the first attempt, playing, with the player at full health;
  - while playing, a session step is a world step;
  - the player is dead in the step their health reaches 0, and the world is frozen from then on (player, boss and the fight's clock do not change);
  - a dead player's world remembers what killed them;
  - the fight is won in the step the last bullet of the second phase lands, and the world is frozen from then on (no movement, no shots, no time);
  - when the boss and the player fall in the same step, the fight is won;
  - restart is refused while playing, and inside the delay after a death or a victory;
  - after the delay a retry gives a fresh world (full health, time 0, the player and the boss back at the start);
  - every retry after a death is the next attempt;
  - every attempt gets its own seed, and the session's seed decides them all;
  - a new fight after a victory starts the attempts over, while the victory screen still shows the attempt that won;
  - standing still in front of the boss ends in death, whatever the seed (20 seeds), and with a cause of death recorded.
- `tests/core/WorldTests.cpp`: a bullet that hits counts toward accuracy, and a miss does not; a touching hairball's hit is recorded as the hairball's.
- `tests/core/BossTests.cpp`: every attack's hit is recorded as that attack's (see [The boss](BOSS.md#tests)).

The end screens and the restart buttons are not unit tested, because they need a window. I checked the screens in Release and Debug builds with window captures: standing still until "YOU DIED" (it read "Attempt 1. You lasted 0:07." and "The Matriarch kept 100% of her health."), both screens halfway through the fade, and "VICTORY" after an `F4` key message sent straight to the window. Retrying from each screen, and winning with real bullets, I check by playing. With the cause of death, three Release runs of a sidestep and then standing still read "Attempt 1. Slain by her touch after 0:09.", "Slain by her touch after 0:08." and "Slain by her Leap after 0:09."

## Limitations

- **There is no title screen.** The game starts playing as soon as the window opens. The main menu comes on day 11.
- **The victory comes the moment the second bar is empty.** There is no death animation for the boss yet. It comes with the game feel pass.
- **There is no best time.** The end screens show only the last fight, and nothing is saved between launches. The attempt count is lost when the game closes.
- **After `F4` the victory screen reads "0 of 0 shots on target (0%)".** The shortcut skips the shooting, so there is nothing to count. It only exists in Debug builds.
- **Esc still quits at once, even in the middle of a fight.** Pause comes on day 11.
