# Game feel

## What it is

The fight has to feel heavy. A Charge that lands should hurt to watch, a broken shield should feel like a reward, every bullet should leave a mark, and the boss coming down from a Leap should shake the room and throw dust across the stone. This doc covers what is in place so far: the hit stop, the screen shake and the particles, which react to events the simulation records (the sounds that come next will react to the same events); health bars that show what a hit took, red at the edges of the screen when the player is hurt; and a death for the boss.

The code:

- `src/core/WorldEvent.h`: `WorldEvent` and `WorldEventKind`, what happened, where and which way.
- `src/core/World.h`, `src/core/World.cpp`: `World::events`, and where each event is recorded.
- `src/core/HitStop.h`, `src/core/HitStop.cpp`: the slow-down.
- `src/core/ScreenShake.h`, `src/core/ScreenShake.cpp`: the shake.
- `src/core/ImpactFeel.h`, `src/core/ImpactFeel.cpp`: the table of how hard each event slows and shakes.
- `src/core/Particle.h`, `src/core/Particle.cpp`: `Particle`, `ParticlePool` (512 slots), drag, vertical arc, floor bounce and wall confinement.
- `src/core/ParticleEmitter.h`, `src/core/ParticleEmitter.cpp`: what each event and continuous state spawns, using its own `Random`.
- `src/app/Game.cpp`: reads the events after each step and runs the slow-down, the shake and the particles.
- `src/app/render/View.cpp`: `WorldCamera` moves and turns the world's camera by the shake.
- `src/app/render/ParticleDraw.h`, `src/app/render/ParticleDraw.cpp`, `src/app/render/WorldDraw.cpp`: draws settled floor litter, dust and airborne bursts in their layers.
- `src/core/TrailingBar.h`, `src/core/TrailingBar.cpp`: the pale trail behind a health bar, and the tally of a run of hits.
- `src/app/render/Hud.cpp`: draws the trails and the boss's tally.
- `src/app/render/ArenaDraw.cpp`: `DrawPainVignette`, the blood at the edges of the screen.
- `src/app/render/BossDraw.cpp`: `DrawDyingBoss`, the boss's death.
- `src/core/Session.h`, `src/core/Session.cpp`: `kVictoryRevealSeconds` and `EndScreenSeconds`, which hold the VICTORY screen back while the boss dies.

## How it works

**Events.** `StepWorld` adds a `WorldEvent` to `World::events` whenever something happens that the player should feel:

| Event | Recorded when | Where, and which way |
|---|---|---|
| `ShotFired` | The player fires | The muzzle, along the shot; `amount` is the gun's damage |
| `DashStarted` | The player starts a dash | Where the dash left from, along the dash |
| `PlayerHurt` | A hit lands on the player and leaves them alive | The player; `amount` is the damage |
| `PlayerKilled` | The hit that empties the player's bar | The player; `amount` is the damage |
| `BossHurt` | A bullet takes health from the exposed boss | Where it hit, along the bullet; `amount` is the damage |
| `ShieldStruck` | A bullet bounces off the Shell or is soaked up by the Barrier | Where it hit, along the bullet; `amount` is 0 on a bounce, or the bullet's damage |
| `ShieldBroken` | The bullet that breaks the shield | Where it hit, along the bullet |
| `HairballKilled` | A bullet kills a hairball | The hairball, along the bullet |
| `BossLanded` | The boss comes down from a Leap | Where it landed |
| `BossHitWall` | A Charge ends against a wall | The boss, along the run |
| `FirstPhaseEnded` | The first bar runs out and the transformation starts | The boss |
| `BossRoared` | The roar at the end of the transformation, or at the start of the frenzy | The boss |
| `BossDefeated` | The second bar runs out | The boss |

Every hit on the player already went through one function (`HurtPlayer`), so one line there covers every source of damage. The boss reports a crash into a wall the way it reports its other actions, with a flag in `BossActions`. The Debug shortcuts (F2, F3, F4) record the same events the real thing would, so they can be used to tune the feel.

**Reading them.** After each step that runs, `Game::FeelEvents` looks each event up in `FeelOf` (`ImpactFeel.cpp`), starts the hit stop, adds the trauma, hands the event to `EmitEventParticles` (`ParticleEmitter.cpp`), and clears the list. While playing, each step also calls `EmitContinuousParticles` for the steady trails that do not come from one instant: dust under an active dash, and hair falling from the boss after her first phase.

**Hit stop.** A hit drops the fight to 8% of its speed, and over the length of the slow-down it eases back to full speed along a smoothstep curve: it stays slow for the first moments, so the hit lands, and reaches full speed without a jolt. `Game::Frame` feeds the fixed timestep the frame's time multiplied by `TimeScale`, so the world takes fewer of its fixed steps, each still 1/60 s long, and the rules play out exactly as at full speed. The drawing keeps blending between the last two steps, so every frame still moves a little. The slow-down itself runs on real time. If a stronger hit lands during a slow-down it takes over from the slowest point; a weaker one adds nothing. Over a slow-down the fight loses about half its length: a hurt player's 0.18 s costs about 0.1 s of the fight.

**Screen shake.** `ScreenShake` holds a trauma from 0 to 1. Events add to it, and it wears off at 1.5 a second. The shake is trauma squared, so a light hit barely moves the screen and a heavy one throws it. The view moves up to 22 units on each axis and turns up to 1.2 degrees, in a direction read from smooth noise: a lattice of fixed random values for each whole step along the noise's line, blended with a smooth curve, at 18 new directions a second. Only the world's camera shakes. The HUD stays still, and the aim goes through the camera without the shake, so a shake never moves a shot.

**Particles.** `Particle` (`Particle.h`) is plain data in an `ObjectPool<Particle, 512>` owned by `Game` through `ParticleEmitter`. Each particle has a spot and velocity across the floor with exponential drag (`e^(-drag * dt)`), and, separate from that, a `height`, `lift`, `gravity` and `bounce` above the floor. Separating height from floor position lets a falling casing, drop, shard or lock of hair cast a small shadow straight under itself while it arcs, bounce until its rebound speed drops below 20 units/s, and then lie flat on the stone (`IsOnFloor`) until its last quarter of life fades it out. Particles also ricochet off the arena walls so casings and shards stay inside the room. `Game::Frame` steps live particles once per frame by `min(frameSeconds, 0.25) * TimeScale`, before running the frame's fixed steps: they stay smooth at high refresh rates, slow down with the world during a hit stop, and draw on their first frame right at the muzzle or impact point where they were born. Restarting clears the pool.

What each moment throws:

- **Every shot (`ShotFired`):** forward muzzle sparks, a puff of rising gunsmoke, and a spent brass casing ejected from behind the barrel to the hunter's right-rear, bouncing twice on the stone and lying there for 4 to 5.5 s (slightly larger and brighter for the pistol than the SMG).
- **Bullet on the shield (`ShieldStruck`):** blue (`Shell`) or orange (`Barrier`) sparks. A bullet that bounces off the Shell (`amount == 0`) sprays a tight specular ricochet with no chips; one that takes strength off the shield fans wider and knocks loose two small shards.
- **Broken shield (`ShieldBroken`):** 28 spinning shards around the shield's ring (`radius * 1.5 + 6`) and 18 sparks in the broken shield's colour.
- **Bullet on the exposed boss (`BossHurt`):** dark maroon blood drops that splat on the floor without bouncing, and torn locks of the Matriarch's hair that flutter down around the wound.
- **Dash (`DashStarted` and dashing):** stone dust kicked back from the takeoff spot, plus a trail of smaller puffs every 0.02 s while the burst lasts.
- **Leap landing (`BossLanded`):** an expanding ring of 28 stone-dust clouds and low grit sparks around the crater.
- **Killed hairball (`HairballKilled`):** a burst of 18 lighter brown hairball locks carried slightly along the killing bullet, with dark blood drops and dust.
- **Other moments:** `BossHitWall` throws dust, sparks and boss hair back off the wall; `PlayerHurt` and `PlayerKilled` spill bright crimson drops; `FirstPhaseEnded`, `BossRoared` and `BossDefeated` burst locks of hair and dust from the boss's body.
- **Phase 2 and Frenzy (`EmitContinuousParticles`):** once the Matriarch leaves Phase 1, locks of her hair shed continuously onto the floor while she is alive and on the ground: 9 a second in Phase 2, 15 while transforming, and 22 in a frenzy, carrying a quarter of her velocity so a charge leaves fallen hair in her wake.

**Health bars that show the hit.** As in Dark Souls, a hit takes the bar down at once, but a strip of pale old gold stays where the bar was. It waits 0.6 s, and every new hit in that time starts the wait again, then it drains at 60% of the whole bar per second until it meets the bar. A heal that passes the trail lifts it with the bar. `TrailingBar` follows a health value step by step and never changes one; `Game` keeps one for the player and one for the boss and steps them after the world. The boss's bar also shows the damage of the current run of hits right of its end, "124", which stays while the hits keep coming and fades as the trail drains. While the boss transforms, its trail is hidden, because the bar is refilling.

**Blood at the edges.** A hit on the player turns the edges of the screen red, fading over the 0.8 s that the hit protects them, squared so it leaves fast. Below 30% health the edges beat red like a slow heartbeat, a sharp beat and a long rest, a little faster than once a second and stronger the lower the health. It is the vignette texture tinted with the blood colour, over the world and under the HUD.

**The boss's death.** The killing blow flashes her white, and the victory's slow-down and the burst of hair start. She convulses and swells for 0.6 s with her mouth wide open, while the colour drains out of her; then she sags, a little wider than long, her eyes shut, and she lies grey on the floor. It lasts 1.4 s and is read from the session's clock in `Victory`, so it plays in slow motion with the slow-down. The VICTORY screen waits 1.5 s (`kVictoryRevealSeconds`) before it starts to fade in, so nothing covers the death, and the restart delay counts from then.

`DrawWorld` draws particles in three passes so nothing hides what matters: settled floor litter (`DrawFloorParticles`) goes right after the torchlight and under every telegraph, shockwave and body; airborne shadows and dust (`DrawUnderbodyParticles`) go over telegraphs and under bodies; sparks, gunsmoke and airborne casings, blood, hair and shards (`DrawOverbodyParticles`) go over floor bodies so impacts pop on top of what they hit.

## Design decisions

- **The simulation records events; the game decides what they look and feel like.** The rules stay free of presentation, the events can be tested without a window, and sounds can be added next by reading the same list, with no change to the rules.
  - Rejected: calling the shake or spawning particles from inside `StepWorld`. `horde_core` would then hold presentation state, and a test of a hit would have to step around camera shakes and visual debris.
  - Rejected: comparing the world before and after each step (health went down, so the player was hit). It misses everything that leaves no trace in the state, like a bullet bouncing off the Shell, and the game would have to know the rules to work it out.
- **The game clears the list, not the world.** The Debug shortcuts change the world between steps, and their events must still be read. If `StepWorld` cleared the list at the start of each step, they would be lost before the game saw them. A list that nobody clears (the tests, mostly) fills up at 64 and drops new events, which changes nothing in the fight.
- **Particles own their own `Random` and live outside `World`.** If `ParticleEmitter` drew numbers from `World::random`, changing a burst count or skipping a frame would shift every boss attack pick after it and break seed determinism. Giving `ParticleEmitter` its own PCG32 stream keeps `World::random` bit-for-bit identical whether particles run or not.
- **Exact exponential drag and a separate `height` axis.** Integrating `e^(-drag * dt)` analytically lands a particle in the same spot at 60 Hz and 165 Hz and can never flip its velocity on a slow frame. Keeping `height` separate from `position` gives falling casings and locks a real ground spot for a shadow and floor bounce in top-down 2D.
- **Three draw layers, and tapered wisps for hair.** Debris drawn before `DrawTorchLight` dimmed by 70% away from the hunter and snapped brighter while airborne than on landing; drawing settled litter right after `DrawTorchLight` and under telegraphs keeps its brightness steady while warnings and bodies always sit cleanly over it. Hair particles are drawn as curved, tapered wisps rather than spiky balls so a shed lock can never be mistaken for a hostile strand.
- **Slow motion that eases out, not a freeze.** My first version froze the world for a whole number of steps (4 for a touch, 18 for a death). Playing it, the freezes read as lag spikes: the picture stopped dead, and the world, drawn without interpolation while frozen, jumped a fraction of a step at the start of each one, which is exactly what a dropped frame looks like. Slowing the clock instead keeps every frame moving, and the eased return removes the snap back to full speed.
  - Rejected: shorter freezes. They feel weaker and still stop the picture dead, so they still read as a hitch, only a shorter one.
  - Rejected: a slow-down that ends at once. The jump from a crawl to full speed is as abrupt as the start of a freeze.
- **The slow-down changes the clock, never the step.** The world always steps 1/60 s at a time, so a hit stop cannot change what happens in the fight, only how fast it is shown. The fight's clock on the end screen counts the fight's own time.
- **The stronger slow-down wins.** Adding them up would let the SMG, or a crowd of hairballs, keep the fight slow for as long as they keep landing.
- **Only rare events slow or shake.** Shots, dashes and bullets on the boss come many times a second. Shaking on each would blur into a constant tremor that hides the hits that matter, so they get no slow-down and no shake; particles carry them now, and sounds will join them next.
- **The boss's weight shakes but does not slow the fight.** A landing and a roar happen whether or not they hit the player, and the player has to keep moving through them, so they only shake. A crash into a wall gets a short slow-down, because it opens a window to punish. Hits on the player, breaks and kills slow the fight, because those are the moments to read.
- **Trauma squared, with smooth noise.** This is the approach from Squirrel Eiserloh's talk on camera shake. Squaring gives small hits a small shake and big ones a big one, from one number. Noise makes the view sway instead of jumping between random offsets every frame, and it does not change with the frame rate.
  - Rejected: a random offset every frame. At 165 FPS it reads as the picture vibrating, not the room shaking.
- **The trail is how much the hits took, not how long ago they landed.** The tally is the trail's own number: the damage since the trail last caught up with the bar. With the SMG it climbs for as long as the stream keeps landing, which is the number worth showing.
  - Rejected: a number per bullet floating off the boss. Ten a second would be clutter, and the boss's bar is where the player already looks for her health.
- **The blood at the edges never covers the middle.** The vignette is clear in the middle of the screen, where the player and the boss are, so the flash says "you were hit" without hiding the next attack.
  - Rejected: a red flash over the whole screen. It hides the attack that comes after the hit, which is the one that kills.
- **The boss dies on screen before VICTORY covers her.** The death is the reward for the whole fight. The screen waits for it; a death screen does not, because after a death the player wants to try again.
  - Rejected: letting the world run during the death. The boss's death would then be part of the rules, and the fight's clock on the screen would keep counting.
- **All three live in `Game`, not in `World`.** Hit stop, screen shake and particles change how the fight is shown and when it steps, never what happens in it. Their data and update math still live in `horde_core`, with unit tests.

## How to tune it

| What | Where | Now |
|---|---|---|
| Slow-down and trauma for each event | `FeelOf` (`ImpactFeel.cpp`) | Hurt 0.18 s / 0.45; heavy hit 0.3 s / 0.65; death 0.6 s / 0.8; shield broken 0.25 s / 0.55; hairball killed 0.1 s / 0.2; landing 0 / 0.5; wall 0.12 s / 0.55; roar 0 / 0.7; first bar 0.45 s / 0.6; victory 0.8 s / 0.9 |
| How slow the fight gets at the hit | `HitStopStats::slowestScale` (`HitStop.h`) | 0.08 of full speed |
| What counts as a heavy hit | `kHeavyHitDamage` (`ImpactFeel.h`) | 30: the Charge and the Leap's impact |
| How far and how fast the screen shakes | `ScreenShakeStats` (`ScreenShake.h`) | 22 units, 1.2 degrees, wears off at 1.5 a second, 18 directions a second |
| How many events a step can hold | `kMaxWorldEvents` (`World.h`) | 64 |
| Particle pool capacity | `kMaxParticles` (`Particle.h`) | 512 |
| Floor settle threshold, floor friction, wall bounce | `kMinBounceLift`, `kFloorDrag`, `kWallBounce` (`Particle.cpp`) | 20 units/s, 14/s, 0.4 |
| Burst counts, speeds, lifetimes and shedding rates | `ParticleEmitter.cpp` | Dash dust every 0.02 s; boss hair 9/s in Phase 2, 15/s transforming, 22/s in Frenzy |
| The trail's wait and drain | `TrailingBarStats` (`TrailingBar.h`) | 0.6 s, 60% of the bar per second |
| Blood at the edges | `kHurtFlash`, `kLowHealth`, `kHeartbeat`, `kHeartbeatsPerSecond` (`ArenaDraw.cpp`) | 0.4 after a hit, below 30% health, 0.25, 1.1 beats a second |
| The boss's death | `kBossDeathSeconds` (`BossDraw.h`), `kDeath...` (`BossDraw.cpp`) | 1.4 s; convulsion 0.6 s, shake 5 units, swell 12% |
| How long VICTORY waits for the death | `kVictoryRevealSeconds` (`Session.h`) | 1.5 s |
| Particle colours and shapes | `Palette.h`, `ParticleDraw.cpp` | `kDust`, `kGunSmoke`, `kDarkBlood`, `kBrass`, `kShell`, `kBarrier` |

The F1 overlay in a Debug build shows the live particle count (`part`), and F2, F3 and F4 break the shield, transform or frenzy the boss, and defeat her at once.

## Tests

- `tests/core/WorldEventTests.cpp`: a quiet step records nothing; a shot records the muzzle, the direction and the damage; starting a dash records its origin and direction once; a hit on the player records the damage, and the last one records the death instead; a dash through a strand records no hit; bullets on the shield record 0 on a bounce, the damage on an absorbed hit, the break and then a hurt boss; a killed hairball records where it died; the first bar, the roar and the victory are recorded; the Debug shortcuts record what the real thing would; a Charge into a wall and a Leap's landing are recorded; the list fills up at 64 without changing the fight, and takes events again once cleared.
- `tests/core/HitStopTests.cpp`: full speed without a hit; a hit drops the speed to the slowest scale, never to 0, and it climbs back to 1 without ever going down or jumping by more than 0.08 in a frame at 144 Hz; the fight loses about half the slow-down's length; a stronger hit takes over from the slowest point, and a weaker one changes nothing.
- `tests/core/ScreenShakeTests.cpp`: no shake without trauma; trauma stops at 1; it wears off at its rate, and the clock starts over once it is gone; the shake grows with trauma squared and never passes its limits; it moves only a fraction of its range from one frame to the next; the same shake always moves the same way.
- `tests/core/ImpactFeelTests.cpp`: shots, dashes and bullets on the boss neither slow nor shake; a heavy hit slows and shakes more than a light one, and the death more than both; the boss's landing and roar shake without slowing; the victory is the biggest moment, and no slow-down is longer than 1 s.
- `tests/core/TrailingBarTests.cpp`: a fresh bar has no trail; a hit leaves the trail where the bar was, waits 0.6 s and drains at its rate down to the bar; hits that keep coming hold the trail and add up the tally, and a hit after it caught up starts a new tally; the drain never goes below the bar, even on a 5 s frame; a heal lifts the trail only once it passes it.
- `tests/core/SessionTests.cpp`: a death shows its screen at once; a victory shows it only after the boss's death, and restart waits for that and the delay (see [Game flow](GAME_FLOW.md#tests)).
- `tests/core/ParticleTests.cpp`: `LifeLeft` clamping; countdown and removal of expired particles; exact exponential drag matching across step sizes and never flipping velocity on a multi-second frame; floating smoke growth and spin; casing floor bounces, spin loss and floor friction once settled; zero-bounce blood sticking on first contact; wall ricochets inside an arena; and a full 512-slot pool dropping new particles cleanly without corrupting live ones.
- `tests/core/ParticleEmitterTests.cpp`: 360 steps of fighting with and without particles leave `World::random` and every outcome identical; shots spawn sparks, smoke and a right-ejected casing that settles on the floor; shield strikes and breaks match the Shell or Barrier colour (bounced shots drop no chips); exposed hits spawn dark boss blood and hair; dashes, leap landings and killed hairballs each spawn their burst; the boss sheds no hair in Phase 1, sheds in Phase 2 and sheds more than twice as fast in Frenzy; and emitting into a full pool caps cleanly at 512.

## Limitations

- **Stills only show part of it.** Screenshots catch the casings on the stone, the sparks, the shards and the dust ring, while the slow-down, the shake and the bounce of a casing can only be judged in motion.
- **No setting to turn the shake down.** Some players get sick from screen shake. A slider belongs in the options, which come with the menus.
- **The shake has no direction.** A hit from the left shakes the screen the same way as one from the right. A kick away from the hit would read better; the events already carry the direction for it.
- **Floor litter fades after a few seconds.** Casings, blood splats and shed hair fade out after 3 to 5.5 s so the 512-slot pool always has room for live bursts without heap allocation.
- **The Debug F4 shortcut shows a tally of 1200.** It takes all of the boss's health at once, so the whole bar trails off as one hit. In a real fight the last run of hits is a few bullets.
- **The blood at the edges cannot be turned down.** Like the shake, it belongs in the options that come with the menus.
- **No sound yet.** Gunfire, bounces, breaks and landings are still silent until the audio pass.
