# Game feel

## What it is

The fight has to feel heavy. A Charge that lands should hurt to watch, a broken shield should feel like a reward, and the boss coming down from a Leap should shake the room. This doc covers the first two tools for that: the hit stop, a moment of slow motion when a heavy hit lands, and the screen shake. Both react to events the simulation records, and the particles and sounds that come next will react to the same events.

The code:

- `src/core/WorldEvent.h`: `WorldEvent` and `WorldEventKind`, what happened, where and which way.
- `src/core/World.h`, `src/core/World.cpp`: `World::events`, and where each event is recorded.
- `src/core/HitStop.h`, `src/core/HitStop.cpp`: the slow-down.
- `src/core/ScreenShake.h`, `src/core/ScreenShake.cpp`: the shake.
- `src/core/ImpactFeel.h`, `src/core/ImpactFeel.cpp`: the table of how hard each event slows and shakes.
- `src/app/Game.cpp`: reads the events after each step and runs both.
- `src/app/render/View.cpp`: `WorldCamera` moves and turns the world's camera by the shake.

## How it works

**Events.** `StepWorld` adds a `WorldEvent` to `World::events` whenever something happens that the player should feel:

| Event | Recorded when | Where, and which way |
|---|---|---|
| `ShotFired` | The player fires | The muzzle, along the shot |
| `PlayerHurt` | A hit lands on the player and leaves them alive | The player; `amount` is the damage |
| `PlayerKilled` | The hit that empties the player's bar | The player; `amount` is the damage |
| `BossHurt` | A bullet takes health from the exposed boss | Where it hit, along the bullet |
| `ShieldStruck` | A bullet bounces off the Shell or is soaked up by the Barrier | Where it hit, along the bullet |
| `ShieldBroken` | The bullet that breaks the shield | Where it hit, along the bullet |
| `HairballKilled` | A bullet kills a hairball | The hairball, along the bullet |
| `BossLanded` | The boss comes down from a Leap | Where it landed |
| `BossHitWall` | A Charge ends against a wall | The boss, along the run |
| `FirstPhaseEnded` | The first bar runs out and the transformation starts | The boss |
| `BossRoared` | The roar at the end of the transformation, or at the start of the frenzy | The boss |
| `BossDefeated` | The second bar runs out | The boss |

Every hit on the player already went through one function (`HurtPlayer`), so one line there covers every source of damage. The boss reports a crash into a wall the way it reports its other actions, with a flag in `BossActions`. The Debug shortcuts (F2, F3, F4) record the same events the real thing would, so they can be used to tune the feel.

**Reading them.** After each step that runs, `Game::FeelEvents` looks each event up in `FeelOf` (`ImpactFeel.cpp`), which gives the length of a slow-down and an amount of trauma, starts the hit stop and adds the trauma, then clears the list.

**Hit stop.** A hit drops the fight to 8% of its speed, and over the length of the slow-down it eases back to full speed along a smoothstep curve: it stays slow for the first moments, so the hit lands, and reaches full speed without a jolt. `Game::Frame` feeds the fixed timestep the frame's time multiplied by `TimeScale`, so the world takes fewer of its fixed steps, each still 1/60 s long, and the rules play out exactly as at full speed. The drawing keeps blending between the last two steps, so every frame still moves a little. The slow-down itself runs on real time. If a stronger hit lands during a slow-down it takes over from the slowest point; a weaker one adds nothing. Over a slow-down the fight loses about half its length: a hurt player's 0.18 s costs about 0.1 s of the fight.

**Screen shake.** `ScreenShake` holds a trauma from 0 to 1. Events add to it, and it wears off at 1.5 a second. The shake is trauma squared, so a light hit barely moves the screen and a heavy one throws it. The view moves up to 22 units on each axis and turns up to 1.2 degrees, in a direction read from smooth noise: a lattice of fixed random values for each whole step along the noise's line, blended with a smooth curve, at 18 new directions a second. Only the world's camera shakes. The HUD stays still, and the aim goes through the camera without the shake, so a shake never moves a shot.

## Design decisions

- **The simulation records events; the game decides what they look and feel like.** The rules stay free of presentation, the events can be tested without a window, and particles and sounds can be added later by reading the same list, with no change to the rules.
  - Rejected: calling the shake from inside `StepWorld`. `horde_core` would then hold presentation state, and a test of a hit would have to step around a camera shake.
  - Rejected: comparing the world before and after each step (health went down, so the player was hit). It misses everything that leaves no trace in the state, like a bullet bouncing off the Shell, and the game would have to know the rules to work it out.
- **The game clears the list, not the world.** The Debug shortcuts change the world between steps, and their events must still be read. If `StepWorld` cleared the list at the start of each step, they would be lost before the game saw them. A list that nobody clears (the tests, mostly) fills up at 64 and drops new events, which changes nothing in the fight.
- **Slow motion that eases out, not a freeze.** My first version froze the world for a whole number of steps (4 for a touch, 18 for a death). Playing it, the freezes read as lag spikes: the picture stopped dead, and the world, drawn without interpolation while frozen, jumped a fraction of a step at the start of each one, which is exactly what a dropped frame looks like. Slowing the clock instead keeps every frame moving, and the eased return removes the snap back to full speed.
  - Rejected: shorter freezes. They feel weaker and still stop the picture dead, so they still read as a hitch, only a shorter one.
  - Rejected: a slow-down that ends at once. The jump from a crawl to full speed is as abrupt as the start of a freeze.
- **The slow-down changes the clock, never the step.** The world always steps 1/60 s at a time, so a hit stop cannot change what happens in the fight, only how fast it is shown. The fight's clock on the end screen counts the fight's own time.
- **The stronger slow-down wins.** Adding them up would let the SMG, or a crowd of hairballs, keep the fight slow for as long as they keep landing.
- **Only rare events slow or shake.** Shots and bullets on the boss come ten times a second with the SMG. Shaking on each would blur into a constant tremor that hides the hits that matter, so they get no slow-down and no shake. They will get particles and sounds instead.
- **The boss's weight shakes but does not slow the fight.** A landing and a roar happen whether or not they hit the player, and the player has to keep moving through them, so they only shake. A crash into a wall gets a short slow-down, because it opens a window to punish. Hits on the player, breaks and kills slow the fight, because those are the moments to read.
- **Trauma squared, with smooth noise.** This is the approach from Squirrel Eiserloh's talk on camera shake. Squaring gives small hits a small shake and big ones a big one, from one number. Noise makes the view sway instead of jumping between random offsets every frame, and it does not change with the frame rate.
  - Rejected: a random offset every frame. At 165 FPS it reads as the picture vibrating, not the room shaking.
- **Both run on real time.** The hit stop decides how fast the fight's clock runs, so it cannot run on that clock itself. The shake is only a view, so it moves on with each frame at full speed, which keeps it smooth on a fast monitor and keeps it shaking through a slow-down.
- **Both live in `Game`, not in `World`.** They change how the fight is shown and when it steps, never what happens in it, and a restart should not have to reset them. Their logic is still in `horde_core`, with tests.

## How to tune it

| What | Where | Now |
|---|---|---|
| Slow-down and trauma for each event | `FeelOf` (`ImpactFeel.cpp`) | Hurt 0.18 s / 0.45; heavy hit 0.3 s / 0.65; death 0.6 s / 0.8; shield broken 0.25 s / 0.55; hairball killed 0.1 s / 0.2; landing 0 / 0.5; wall 0.12 s / 0.55; roar 0 / 0.7; first bar 0.45 s / 0.6; victory 0.8 s / 0.9 |
| How slow the fight gets at the hit | `HitStopStats::slowestScale` (`HitStop.h`) | 0.08 of full speed |
| What counts as a heavy hit | `kHeavyHitDamage` (`ImpactFeel.h`) | 30: the Charge and the Leap's impact |
| How far and how fast the screen shakes | `ScreenShakeStats` (`ScreenShake.h`) | 22 units, 1.2 degrees, wears off at 1.5 a second, 18 directions a second |
| How many events a step can hold | `kMaxWorldEvents` (`World.h`) | 64 |

The F2, F3 and F4 shortcuts in a Debug build break the shield, transform the boss and win the fight, which is the quickest way to feel the big moments.

## Tests

- `tests/core/WorldEventTests.cpp`: a quiet step records nothing; a shot records the muzzle, the direction and the damage; a hit on the player records the damage, and the last one records the death instead; a dash through a strand records nothing; bullets on the shield record a strike, the break and then a hurt boss; a killed hairball records where it died; the first bar, the roar and the victory are recorded; the Debug shortcuts record what the real thing would; a Charge into a wall and a Leap's landing are recorded; the list fills up at 64 without changing the fight, and takes events again once cleared.
- `tests/core/HitStopTests.cpp`: full speed without a hit; a hit drops the speed to the slowest scale, never to 0, and it climbs back to 1 without ever going down or jumping by more than 0.08 in a frame at 144 Hz; the fight loses about half the slow-down's length; a stronger hit takes over from the slowest point, and a weaker one changes nothing.
- `tests/core/ScreenShakeTests.cpp`: no shake without trauma; trauma stops at 1; it wears off at its rate, and the clock starts over once it is gone; the shake grows with trauma squared and never passes its limits; it moves only a fraction of its range from one frame to the next; the same shake always moves the same way.
- `tests/core/ImpactFeelTests.cpp`: shots and bullets on the boss neither slow nor shake; a heavy hit slows and shakes more than a light one, and the death more than both; the boss's landing and roar shake without slowing; the victory is the biggest moment, and no slow-down is longer than 1 s.

I checked the rest by playing a Release build with temporary logs. With the first version (freezes), standing still recorded the Charge's hit, the crash into the wall, the touches and the death, and in Debug, F3, F2 and F4 recorded the first bar, the roar, the break and the victory. With the slow-down, a log of every slow frame showed the speed starting at 0.08 and climbing smoothly to 1, with the world still taking a step every few frames, and the player standing still still died at 7.1 s of the fight's clock. The shake shows in captures as the view turned and moved against a calm frame.

## Limitations

- **Stills do not show it.** The slow-down and the shake can only be judged by playing. The captures show a moved and turned view at best.
- **No setting to turn the shake down.** Some players get sick from screen shake. A slider belongs in the options, which come with the menus.
- **The shake has no direction.** A hit from the left shakes the screen the same way as one from the right. A kick away from the hit would read better; the events already carry the direction for it.
- **Nothing for the bullets that land yet.** Shots and hits on the boss feel the same as before until the particles and sounds arrive.
