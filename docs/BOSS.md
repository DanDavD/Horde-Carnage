# The boss

## What it is

The Matriarch, the one boss the game is about: a giant hairball, mother of the small ones, in the middle of the arena. The whole game is one fight against it, in the spirit of Gun Bros and Dark Souls 1: a boss that is hard on purpose, whose patterns you learn by dying. See the [roadmap](ROADMAP.md#changes-to-the-plan) for why.

This document describes what exists today. The boss stalks the player, shoves them, hurts on touch, soaks up bullets behind an alternating shield (see [The boss's shield](SHIELD.md)), and has the three attacks of its first phase: the Charge, the Hair Ring and the Leap. It picks the next one at random, weighted by the distance to the player and never the same one twice in a row, and sometimes it chains two into a combo. When its health first runs out it transforms, and a second phase starts with a full bar, faster, and with a fourth attack, the Aimed Volley. At half of that bar it falls into a frenzy, and every attack gains something. The second phase also brings the Summon: hairballs that come out of marks on the floor around the player.

The rules live in `horde_core`:

- `src/core/Boss.h`, `src/core/Boss.cpp`: the `Boss` struct, with the same body components as the player and the hairballs (`Motion`, `MovementStats`, a radius and a weight) and the boss's numbers; its attack state (`BossAttack`, `AttackStage`, `ChargeStats`, `HairRingStats`, `LeapStats`); and the systems that run it: `StepBoss`, which returns the `BossActions` the world has to carry out, `IsCharging`, `IsAirborne`, `WarningSeconds`, `LeapCrouchSeconds`, `LeapAirSeconds`, `PlanChargePath`, and the ring's `HairRingStrands`, `HairStrandDirection` and `MakeHairStrand`.
- `src/core/AttackChoice.h`, `src/core/AttackChoice.cpp`: how the boss picks what comes next. `AttackKind`, the weights by distance (`AttackWeights`, `AttackChoiceStats`, `AttackWeightAt`, `ChooseAttack`), and the combos (`ComboStats`, `Combo`, `kCombos`, `ChooseFollowUp`). They draw from the world's seeded generator (see [Random numbers](RANDOM.md)).
- `src/core/Shockwave.h`, `src/core/Shockwave.cpp`: the `Shockwave` the Leap sends out, with `MakeShockwave`, `StepShockwave` and `ShockwaveTouches`.
- `src/core/Shield.h`, `src/core/Shield.cpp`: the boss's `Shield`, described in [The boss's shield](SHIELD.md). `StepBoss` steps it, and `StaggerBoss` and `IsStaggered` handle the stagger when it breaks.
- `src/core/World.h`, `src/core/World.cpp`: `kBossArena` and `kStartingDistance`; `CreateWorld` places the boss; `StepWorld` calls `StepBoss` with the world's `random`; `HurtPlayer` records which attack landed each hit in `World::lastHitBy`, for the death screen; `SpitHairRing` spawns the ring's strands into `World::hostileProjectiles`, where `StepHostileProjectiles` flies them and hurts the player; `LandLeap` applies the Leap's impact and spawns its shockwave into `World::shockwaves`, where `StepShockwaves` grows it; the collision, bullet and contact damage systems include the boss, unless it is dead or in the air (`IsBossABody`); and `CameraFocus` keeps it in view.

The game executable draws it:

- `src/app/Game.cpp`: `DrawBoss` draws the same hairy body as a hairball (`DrawHairBody`), three times bigger, darker, with glowing eyes, plus the crouch, the ghosts and the stun stars of the Charge (`DrawChargingBoss`), the lean of the Volley (`DrawVolleyingBoss`) and its line on the floor (`DrawVolleyTelegraph`), the transformation (`DrawTransformingBoss`), the red eyes of the second phase, the swell of the Hair Ring (`DrawSpittingBoss`), and the jump of the Leap (`DrawLeapingBoss`). `DrawChargeTelegraph`, `DrawHairRingTelegraph` and `DrawLeapTelegraph` draw the warnings on the floor, `DrawHairStrand` draws the strands, and `DrawShockwave` the shockwaves. `DrawBossBar` draws its health along the bottom of the screen, under its name.

## How it works

**The start.** `CreateWorld` puts the boss in the middle of the arena (1640x1040) and the player `kStartingDistance` (400 units) south of it, facing it. There are no hairballs.

**Stalking and attacking.** The boss is always doing one of two things:

```
  Stalking  -- 0.8 to 1.6 s -->  Attack: Warning --> Active --> Recovery
      ^       (2 s the first time)                       |              |
      |                                                  | combo        |
      |                                                  v              |
      |                       Follow-up: Warning --> Active --> Recovery (1.2 s at least)
      |                                                                 |
      +---------------------------- recovery over ----------------------+
```

- **Stalking** (`Boss::attack` is empty): it walks at the player with the same `ChaseInput` as a hairball, stopping at contact, at 120 units/s at most and slow to get going (400 units/s^2). `secondsUntilAttack` counts down. The first stalk lasts 2 s, so the fight opens with a moment to look at her; every later one lasts a random time between 0.8 and 1.6 s. When it runs out the boss starts an attack: `nextAttack` if one is already decided (the fight always opens with a Charge), otherwise one picked by `ChooseAttack` (see "Choosing an attack" below).
- **Attacking** (`Boss::attack` holds a `BossAttack`): every attack runs through the same three stages, `Warning`, `Active` and `Recovery`, and `stageSeconds` is the clock of the current one. When the recovery ends, the boss stalks again. If the attack opened a combo, its recovery never starts: the second attack takes its place (see "Combos" below).

The timings below are each attack's stats. In the fight every warning is stretched or shortened by a random factor between 0.85 and 1.15 (`BossAttack::warningScale`, read through `WarningSeconds`), and the second attack of a combo shortens it again, to 0.7 of that. The two locks keep their exact length: the Charge's aim always locks for the last 0.3 s, and the Leap's shadow for the last 0.35 s.

**The Charge,** stage by stage:

1. **Warning, 0.8 s.** The boss plants itself and crouches. Its path is drawn on the floor: a band as wide as its body, from where it stands to where the run will end. For the first 0.5 s the path follows the player. For the last 0.3 s (`aimLockSeconds`) it is locked, drawn brighter, and the boss trembles. That is the moment to get out of the way.
2. **Active.** The boss runs along the locked direction at 1000 units/s for up to 800 units. Each step `PlanChargePath` works out where the run ends: after the full 800 units, or earlier, where the body meets a wall. While it runs, the boss passes through the player instead of shoving them, still bowls hairballs aside, and a touch takes 30 instead of 20 and throws the player sideways out of the path at 700 units/s.
3. **Recovery.** If the run ended at a wall, the boss is stunned for 1.5 s, with stars circling over its head: the opening to punish it. If it ended in the open, it recovers in 0.5 s.

**The Hair Ring,** stage by stage:

1. **Warning, 0.6 s.** The boss stops and swells. A red ring on the floor around it marks where the strands will leave, and the gap is a pale lane that runs 420 units out from the body. The gap opens 90 degrees around from the direction of the player at the start of the warning: on one side, and on the other side the next time (`ringsStarted`). It does not move after that.
2. **Active, 0.15 s.** On its first step the boss spits: `StepBoss` returns `spitHairRing`, and `SpitHairRing` spawns one strand for each of 48 directions 7.5 degrees apart, minus the 6 around the gap. That is 42 strands and a 45 degree gap. Each strand leaves the edge of the body at 220 units/s, with a radius of 8, and flies 900 units at most.
3. **Recovery, 0.8 s.** The boss stands still, deflated: the opening to punish it.

**The Leap,** stage by stage:

1. **Warning, 1.35 s.** The boss crouches for 0.35 s. Its shadow appears under the player, with the edge of the landing zone (100 units around it) drawn faintly on the floor. Then the boss leaves the ground for 1 s. In the air it is not a body: it does not collide, take bullets or hurt on touch, and it is drawn above everything, in an arc over the floor. Its shadow grows and follows the player at 200 units/s, a third slower than the player runs, then locks for the last 0.35 s, and the zone's edge turns bright.
2. **Active, 0.15 s.** It lands on the shadow: `StepBoss` returns `landed`, and `LandLeap` hurts a player inside the landing zone for 35 and throws them out at 600 units/s. A shockwave leaves the edge of the body: a ring that grows at 380 units/s, faster than the player runs, up to 420 units. Its band, 20 units wide, hurts for 20 once.
3. **Recovery, 1 s.** The boss sits where it landed: the opening to punish it.

The landing zone can be escaped on foot only by moving from the start of the jump: the shadow is slower than the player, so it falls behind. A player who only starts running when it locks covers about 100 units before the landing, not enough to leave the 120 unit reach of the zone (its radius plus the player's), so then only the dash helps. The shockwave cannot be outrun at all: it is dashed through, or it reaches the end of its range first.

**Choosing an attack.** When a stalk ends, `ChooseAttack` picks the next attack from the distance between the centres of the boss and the player at that moment. Each attack has a weight at three distances, and between them the weight blends in a straight line:

| Attack | Near (250 or less) | Middle (400) | Far (550 or more) |
|---|---|---|---|
| Charge | 1 | 2 | 4 |
| Hair Ring | 4 | 2 | 1 |
| Leap | 1 | 4 | 2 |

The attack that ended last (`Boss::lastAttack`) gets a weight of 0, so it never comes twice in a row outside a combo. Up close that makes two picks in three a Hair Ring; far away, four in seven a Charge, or two in three a Leap right after a Charge. So no distance is safe: hugging the boss invites the Ring, running far invites the Charge.

**Combos.** When an attack starts, `ChooseFollowUp` decides whether it opens a combo: with a 35% chance, if `kCombos` has one that starts with it. There are three:

| Combo | Opener, then follow-up | What it asks of the player |
|---|---|---|
| The Rebound | Charge, then Charge | The second run comes back from the wall at the player. Dodging the first one is not the end of it. |
| The Pursuit | Charge, then Leap | The boss leaps after a player who ran far from the charge. |
| The Crash | Leap, then Hair Ring | A ring on landing: dash through the shockwave, then find the gap. |

The opener plays as usual until its recovery would start. In that step the follow-up starts instead (`BossAttack::followUp`), so there is no opening between the two, and a Charge stunned by a wall shakes the stun off. The follow-up (`BossAttack::endsCombo`) has a warning 0.7 times as long, and a recovery of at least 1.2 s, the reward for getting through the whole chain. A Hair Ring opens no combo, and a follow-up opens none either, so a combo is always two attacks.

**Every random pick** draws from `World::random`, in a fixed order: the attack (when it is not already decided), then the warning's factor, then the combo, and the next stalk when the attack ends. So the seed and the player's inputs decide the whole fight.

**The strands** fly in their own pool, `World::hostileProjectiles`. Every step, after contact damage, `StepHostileProjectiles` moves each one and sweeps it against the player with `SweepCircle`, the test bullets use. A dashing player is not there for them: the strand flies on. Otherwise the strand hurts the player for 15 through `HurtPlayer` and is spent, even when the invulnerability window keeps it from hurting. A strand is also gone at a wall or at the end of its range. Strands ignore hairballs, the boss and the player's bullets.

**Every fixed step,** after the player and the hairballs have moved:

1. **Think and move.** `StepBoss` stalks, picks the next attack, or advances the attack's stage, and tells the world when to spit a ring or a burst, when it lands and when it roars.
2. **Collide.** The collision passes treat the boss as one more body (see [Collisions](COLLISIONS.md)). At weight 40 against the player's 4, the player takes 91% of any overlap, so the boss shoves the player and the player cannot shove it. A dashing player passes through it, and a charging boss passes through the player.
3. **Take bullets.** For each bullet, `FirstHit` sweeps it against every hairball and the boss, and the body it touches first takes it. `HitBoss` hands the bullet to the shield first (see [The boss's shield](SHIELD.md)): while it is up, the bullet bounces off or wears it down, and the bullet that breaks it staggers the boss for 1 s. Only while it is down does the bullet take the boss's health and start a 0.1 s flash. No knockback.
4. **Hurt on touch.** `ApplyContactDamage` checks the boss before the hairballs. A touch takes 20, or 30 plus the knockback while it charges, through `HurtPlayer`, the single door for damage (see [Health](HEALTH.md)). A dash protects from both.
5. **Fly the strands and grow the shockwaves.** After the bodies, so when the boss's body and a strand or a wave reach the player in the same step, the heavier hit is the one that lands. A wave hurts once, the first time its band reaches the player; like a strand, it passes a dashing player by, and any other touch spends it, even inside the invulnerability window. So a player caught by the impact is not hit again by the wave that leaves under them.

**The camera** follows `CameraFocus`: the player, pulled 40% of the way toward the boss, but never more than 200 units. See [Camera](CAMERA.md).

**The Aimed Volley,** second phase only, stage by stage:

1. **Warning, 0.55 s.** The boss holds its ground and leans back, with a thin red line on the floor from its body toward the player. The line, like the boss's aim, follows the player the whole time.
2. **Active.** Three bursts, 0.3 s apart, the first as the warning ends. Each burst is five strands 8 degrees apart, a fan 32 degrees wide (`StepBoss` returns `spitVolley`, and `SpitVolley` spawns them into the strands' pool). The bursts take turns: the first and the third go where the player is; the second goes where the player will be if they keep their velocity (`LeadPoint`: the time of flight is guessed from the distance, then four times again from the distance to the last guess, which settles because the strands are faster than the player; at most 1.5 s ahead, and never past a wall). They fly at 520 units/s, more than twice the Hair Ring's, with a radius of 7, and take 12. The invulnerability window lets a burst land only once.
3. **Recovery, 0.8 s,** once 0.15 s have passed since the last burst.

Running in a straight line, the second burst meets the player; stopping, the first and the third do. Only a change of rhythm between bursts, or a dash, gets through all three. `StepBoss` takes the player's `Motion`, not only a position, for the lead. The strands are the same component and pool as the Hair Ring's, and carry `DamageSource::Volley`, so the death screen can tell the two apart.

**The transformation and the second phase.** The boss has three phases of life (`BossPhase`):

```
  One  -- the bar runs out -->  Transforming, 2 s  -- roar -->  Two  -- the bar runs out -->  defeated
```

- **One.** Everything above. The bullet that empties the bar does not kill the boss: `HitBoss` calls `StartTransformation`, which drops its attack and its stagger and refills its health at once.
- **Transforming, 2 s.** The boss stands still, swelling and trembling harder as it goes, its eyes flickering from amber to red. Bullets still hit it, and are spent, but take nothing, and the shield is not drawn. The bar fills back up over the two seconds. Its touch still hurts.
- **The roar.** When the time is up, `StepBoss` returns `roared`, and the world sends a shockwave from the body that reaches 300 units, less than the 420 of a landing, so it catches a player who waited next to the boss. Then `EnterPhaseTwo` swaps the first phase's numbers for `PhaseTwoStats`: a fresh Shell with a shorter opening, and the second phase's choice and combo numbers. Its first attack is always the Volley, so the player sees at once what changed.
- **Two.** The eyes stay red. Stalks last 0.6 to 1.2 s instead of 0.8 to 1.6, warnings are stretched by 0.75 to 1.0 instead of 0.85 to 1.15, half of the Charges and Leaps open a combo instead of 35%, with 1 s of rest after it instead of 1.2, and each broken shield leaves the boss exposed for 4 s instead of 5. The Volley joins the picks, with weights 1, 3 and 3 near, in the middle and far away. Emptying this bar wins the fight: `IsBossDefeated` is true only in the second phase.

**The frenzy.** In the second phase, the bullet that takes the boss to half its health (`ShouldFrenzy`) throws it into a frenzy (`EnterFrenzy`), once: it roars a second 300 unit shockwave, bristles (more tufts) and reddens over a glow on the floor that pulses, and `FrenzyStats` replace its numbers:

| Attack | In the frenzy |
|---|---|
| Charge | While it runs it throws a pair of strands every 0.15 s (`throwBolts`): one at the player, one where the player is going (`LeadPoint`). A run across the arena is about six pairs. |
| Hair Ring | Two rings, 0.4 s apart, the gap turning 30 degrees between them. |
| Leap | The shadow follows at 240 units/s instead of 200, still slower than the player. The landing also spits a ring of 12 strands with no gap, one of them at the player, at the Hair Ring's speed. |
| Aimed Volley | Five bursts 0.25 s apart: aimed, led, aimed, led, aimed. |
| Summon | Six marks instead of four. |
| The rhythm | Stalks of 0.4 to 0.9 s, combos on 65% of the Charges and Leaps, 0.9 s of rest after one. |

The frenzy's strands carry `DamageSource::Frenzy`, so the death screen reads "Slain by her frenzy". In a Debug build, `F3` in the second phase drops the boss to half and starts the frenzy (`FrenzyBoss`).

**The Summon,** second phase only, stage by stage:

1. **Warning, 0.75 s.** The boss holds its ground, swells and shudders: it howls. On the first step, `StepBoss` returns `summon`, and `PlaceSummon` puts four marks on the floor (`World::spawnMarkers`): dark pits with a red rim that closes in on each of them.
2. **Active, 0.2 s,** then **Recovery, 1 s.** The boss is spent, while the hairballs come out.

**Where the marks go.** `PlanSummon` spaces them evenly on a circle 250 units around the player, turned by a random angle from the world's generator, and pushes each one 40 units inside the walls. A mark that ends up closer than 150 units to the player (pushed in from behind a wall) or than 100 units to the boss is dropped, so near a wall or a corner a summon brings three hairballs, or fewer. The circle is around the player, not the boss: running to the far side of the arena does not get away from it.

**The hairballs.** Each mark counts down 0.75 s (`StepSpawnMarkers`, before the collisions), then lets out a hairball where it was, which shakes itself for 0.3 s before it chases, like one that was just hit. They are the hairballs from the start of the project, with the same health (3 pistol or 5 SMG bullets), speed (160 units/s) and touch (10). A summon brings fewer when the arena already holds 8 (`SummonStats::maxAlive`), so ignoring them does not bury the player. Bullets on them do not reach the boss, so every hairball is a choice: shoot it, or outrun it.

**The hairballs spit.** A summoned hairball spits a strand at the player every 3 s (`HairballSpit`, from `StepEnemies`), aimed where the player is, at 300 units/s, for 8. Its first one comes at a random moment between 1 and 3 s after it comes out, so a crowd does not spit in step. For the last 0.4 s before each, it swells and its eyes turn red. The strands fly in the boss's pool and carry `DamageSource::Hairball`. A hairball made any other way (`Enemy::spits` is false by default) never spits.

**Death.** At 0 health in the second phase the boss is out of the simulation: `StepBoss` drops its attack and brakes it to a stop, it is no longer a body in the collisions, bullets fly through it, it does not hurt, and the camera goes back to the player. It is drawn grey with dark eyes. `IsBossDefeated` turns true, so the session ends the fight in the same step with the "VICTORY" screen (see [Game flow](GAME_FLOW.md)).

## Design decisions

- **A type of its own, not an `Enemy` with bigger numbers.** The boss shares every body system with the hairballs, but its rules differ: bullets neither stagger nor push it, it attacks with a warning, an active part and a recovery, and it will change phase. Putting that into `Enemy` would give every hairball fields it never uses, and a flag checked everywhere.
  - Rejected: a boss flag in `Enemy`. Every system would ask "is this the boss?", and the hairball tests would carry the boss's rules.
  - Rejected: a base class with virtual behaviour. As with the hairballs, the difference is in what the body decides to do, not in how it moves, so it belongs in data and systems.
- **Exactly one boss, as a plain member of `World`.** The game is one fight. A pool or a list for one object would only add loops and empty checks.
- **The second phase is new numbers, not new code.** `EnterPhaseTwo` copies `PhaseTwoStats` over the boss's choice, combo and shield numbers; the attacks themselves do not know which phase it is. Making the second phase harder or easier is a change of data, which suits the JSON on day 9.
  - Rejected: an `if (phase == Two)` in each attack. The rules would spread over every attack, and the tests of each attack would need to cover both phases.
- **The bar refills at the start of the transformation.** The health is full from the first step of it, and the drawing fills the bar over the two seconds. No system needs a "health that is coming back".
- **Invulnerable while it transforms, but still a body.** Bullets hit it and are spent, so shooting it reads as a wall, not as a ghost, and the player learns to stop shooting and get ready.
- **The roar reaches 300 units, not the landing's 420.** At 420 it hit the player at the distance the fight starts at, so it punished doing nothing wrong. At 300 it punishes waiting next to the boss for the second phase.
- **The Volley's bursts take turns: aimed, led, aimed.** At first every burst was aimed where the player was. From far away, at 400 units/s, a player who simply kept running sideways was never hit, and fighting from the far side of the arena became the safe answer to the whole second phase. Leading every burst would have the opposite problem: stopping would beat it. Alternating means neither habit works, and the answer is to watch the rhythm, as with a boss in Dark Souls. The speed went from 400 to 520 units/s for the same reason.
  - Rejected: leading every burst. Against a player who keeps running it never misses, and against one who stops it always does, so the answer would be a habit again.
  - Rejected: one guess at the flight from today's distance, capped at 1 s. A player running away on a diagonal outran it by 50 to 80 units, a miss for a burst 27 units from a hit. Four guesses and a 1.5 s cap brought it within 9 units in the four runs I measured (across, away, on a diagonal, close by).
  - Rejected: random spread. It makes the hits feel like luck, which is the opposite of a fight learned by dying.
- **The lead is capped at 1.5 s and kept inside the walls.** Far away the flight is long; leading by all of it would aim at empty floor or into a wall, which teaches nothing.
- **A frenzy at half of the second bar, with something new in every attack.** Played by hand, the second phase was safe from far away, and the last half of the fight felt like the first. The frenzy gives the end of the fight its own shape without new attacks to learn: each extra is a twist on an attack the player already reads, and each one punishes a habit. The Charge's strands punish running from it in a straight line, the Leap's ring punishes dashing the wave and stopping, the double ring punishes running to where the first gap was, and the longer Volley punishes a rhythm that works for three bursts.
  - Rejected: a third phase with its own bar. A third transformation and a third bar would make the fight longer, not harder, and the player would wait through another roar.
  - Rejected: only faster numbers. Shorter warnings past a point stop being readable; new twists keep the warnings honest and make the player think again.
- **The frenzy is numbers too, plus two small rules.** `EnterFrenzy` copies `FrenzyStats` over the attacks' stats. Only the strands of the Charge (`boltIntervalSeconds`) and of the landing (`landingStrands`) needed code, and both are off at 0, which is their value until the frenzy.
- **The Summon rings the player, not the boss.** It is the second phase's answer to keeping far away: the hairballs come out where the player is, so distance alone is not safe, and the boss's other attacks keep coming while the player deals with them. The weights back it: the Summon is picked most far away (2.5, against 0.5 up close).
  - Rejected: marks near the boss. They would only matter to a player who already fights up close, the opposite of what the Summon is for.
  - Rejected: marks at fixed spots by the walls. A player would learn the spots and stand elsewhere.
- **The marks come with the howl, and the hairballs with its end.** The marks are the warning; 0.75 s is enough to step away from one, not to leave the area. The world counts the marks down on its own, so the boss can recover while the hairballs arrive.
- **Summoned hairballs spit.** Without it, a hairball was a slow thing to walk away from, and a player could ignore the crowd and watch only the boss. A strand every 3 s from each one, out of step with the others, makes the player keep the whole arena in view, which is the point of the second phase. The strands are slow and aimed, not led, so they can be read; what makes them hard is how many directions they come from.
  - Rejected: every 1.5 s. With eight hairballs that is more than five strands a second on top of the boss's attacks, and the strands stop being something to read.
  - Rejected: all hairballs spitting together. One volley from the crowd every few seconds is a rhythm to learn, not something to keep an eye on.
- **A cap on living hairballs.** Eight at once is a crowd that slows the player (see [Collisions](COLLISIONS.md)); more would turn the fight into a horde shooter, which is the game I left on day 4.
- **The Volley starts the second phase.** It is the one attack the player has not seen, so showing it first says "this is new" more clearly than any text.
- **A dead boss is inert, not removed.** The `World` always has its boss, so no system needs to ask whether there is one. The tests about the player and the hairballs use a world with a dead boss for the same reason: it is the boss "not being there" by the game's own rules.
- **Every attack has the same three stages.** The player learns one rhythm for the whole fight: watch the warning, survive the attack, punish the recovery. In code, the stages are an `enum` and one clock, and each attack only decides what happens inside them. The Hair Ring reused them as they were, and the Leap will too.
  - Rejected: a state per attack and stage (`ChargeWarning`, `Charging`, `ChargeRecovery`, `RingWarning`...). Five attacks would make fifteen states that repeat the same timing code, and nothing would force them to keep the same rhythm.
  - Rejected: a class per attack with virtual `Update`. The attacks share all their data except a direction, a distance and a count, so a hierarchy would be more code than behaviour, and it would put the attacks in the heap instead of in the plain `Boss` struct.
- **A random choice, weighted by distance.** A fixed order is learned in one attempt, and after that the fight is about rhythm, not reading. A random pick makes the player read every warning, and the weights keep the boss from looking random: each attack shows up where it makes sense, the Charge from afar, the Ring up close and the Leap in between.
  - Rejected: an even pick. The boss would often answer a far player with a Ring that cannot reach them, and the fight would look like dice, not like a creature that hunts.
  - Rejected: a shuffled bag (every attack once, in a random order). It is fairer to the player, but the last attack of a bag is always known, and the distance could not weigh in.
- **Never the same attack twice in a row.** Two Rings in a row read like a stutter, not a decision, and a streak of the same attack would turn a hard fight into a lucky one or an unlucky one. The Rebound is the one exception, and it is a combo on purpose.
- **The pick happens when the stalk ends, not when the last attack ends.** The boss walks for up to 1.6 s between the two, and the distance it was picked for would be stale by then.
- **The fight opens with a Charge.** It is the plainest attack to read: one line on the floor. The first attack is the only one the player can count on.
- **A stalk of random length, and a first one of 2 s.** A fixed stalk is a countdown the player can learn, and a short random one keeps the pressure up: at 0.8 to 1.6 s, there is time for a few shots and a step, and a drink (1 s) is a gamble. The first stalk is longer, so a new attempt does not start in the middle of an attack.
- **Warnings of random length, locks of exact length.** Dark Souls bosses delay their swings so a player who dodges to a rhythm gets caught. A 15% stretch either way does the same, and the moment that decides the dodge (the Charge's aim lock, the Leap's shadow lock) stays the same length, so a player who watches the boss is never cheated.
  - Rejected: random lock times. The lock is the cue to move. If it moved, the warning would stop being a promise.
- **A combo replaces the opener's recovery.** That is what makes it a combo: the opening the player expects is not there. The follow-up has a shorter warning, since the player is still busy with the first attack, and a longer recovery, so getting through the chain pays.
- **Combos are a short, fixed list.** Each one is designed to ask a new question (a run that comes back, a leap that follows a player who ran, a ring that has to be read right after a shockwave), and the warnings still tell the player which one is coming. Chains of any length, or a random follow-up, would surprise the player without teaching them anything.
  - Rejected: a combo after any attack, with any follow-up. A Hair Ring into a Charge would aim the run while the player is still threading between the strands, with no room to step out of the line.
- **The aim locks before the run.** If the path followed the player until the last moment, the only answer would be the dash. With 0.3 s of locked path, a quick sidestep also works, and the bright path and the tremble tell the player exactly when to move.
- **The telegraph draws the real path.** `DrawChargeTelegraph` calls the same `PlanChargePath` that the run follows, so the band on the floor ends exactly where the boss will stop, and the bar across its end appears only when the run will end at a wall.
- **A fixed distance, not "until a wall".** At 800 units the run overshoots the player by a lot and usually ends at a wall, but a charge from near one wall into the open middle stops short and recovers fast. So where the player stands decides how big the opening is, and leading the boss into a wall is a skill.
- **The run goes through the player.** At 1000 units/s a shove would carry the player across the arena and pin them against the far wall. Instead the hit lands once (the invulnerability window covers the rest of the overlap) and the knockback throws the player sideways, out of the path.
- **The ring's gap opens to the side, never toward the player.** If it opened toward the player, standing still would be the answer. At 90 degrees, a player 400 units away has to cover about 370 units in about 2 s (the warning plus the ring's flight), which takes a little over a second on foot. Alternating sides keeps the player from settling into one habit.
  - Rejected: a random gap. It could land behind the boss, where only the dash would work, and the alternation already keeps the player from settling into one habit.
- **The gap is fixed from the start of the warning.** The Charge follows the player and then locks, because a straight line is easy to sidestep. The ring is the opposite: the player needs to know at once where to run, so the lane appears with the warning and stays.
- **The boss decides, the world spawns.** `StepBoss` returns `BossActions` instead of writing into a pool, so `Boss.cpp` knows nothing about the world, and the boss tests check the spit without one.
- **The strands have a pool of their own.** Bullets and strands are the same `Projectile` component and fly the same way. In two pools, `StepBullets` never tests a strand and `StepHostileProjectiles` never tests a hairball, and no hit test checks which side fired.
  - Rejected: one pool with a flag for the side. Every loop would filter on it, and forgetting the filter once would let the ring hit its own boss.
- **A strand that touches the player is spent, even in the invulnerability window.** It reads like a hit. Two strands usually reach the player together; the first one hurts, and the second one disappears without hurting, instead of flying on behind the player.
- **The dash goes through strands.** The ring is the attack that teaches the dash timing: dash into the ring just before it reaches you, and come out inside it, like a roll through an attack in Dark Souls.
- **The ring is slower than the player.** At 220 units/s it is something to read and walk away from, not a bullet. Its danger is its size.
- **Dense close up, sparse far away.** At 250 units from the boss the strands are 33 units apart, which leaves 17 units between them, too narrow for the player's 40 unit body. Past about 430 units the openings are wide enough to slip through. I left it that way: distance is a fair defence, and the Charge punishes standing far away.
- **The Leap's shadow follows, then locks, like the Charge's aim.** The same rule in a new shape: the warning shows where, and it stops following just before the hit, so the answer is to move at the right moment. The shadow is slower than the player, so moving early is a second answer, and the dash is the third.
- **In the air the boss is not a body.** Bullets would hit a boss high above the floor, and a body in the collisions would shove the player around from the sky. Out of reach for a second, it gives the player time to look at the shadow instead of shooting.
- **The landing is two hits, but costs one.** The impact punishes standing under the shadow, and the shockwave punishes standing anywhere near. A player caught by the impact is still blinking when the wave passes, and the wave is spent, so one mistake costs one hit.
- **A shockwave cannot be outrun.** At 380 units/s against the player's 300, the only answers are being far enough away (420 units) or dashing through its band. That makes the Leap the attack that tests the dash timing against something that comes at the player, where the Hair Ring tests it against something to walk through.
- **Shockwaves are a component and a pool of their own.** They grow instead of flying, and they hit with a band instead of a circle, so they are not `Projectile`s. A pool of 8 keeps the second phase's leaps, and any other attack that needs a wave, free of allocations.
- **Waves and a turning gap are already numbers.** `waves`, `waveIntervalSeconds` and `gapTurnDegrees` are 1, 0.4 s and 0 degrees today. Rings in quick succession, or a gap that sweeps around the boss, are for the second phase, and they will need new numbers, not new code. The combos chain whole attacks instead.
- **No stagger and no knockback from bullets.** The SMG hits ten times a second. If each bullet staggered the boss, the SMG would lock it in place, and the fight would be about holding the button. The openings to punish it come from its own attacks (the recovery after each one) and from its shield. The one bullet that staggers it is the one that breaks the shield, once per shield.
- **Heavy, but not immovable.** Weight 40 means the boss wins every shove, which is what a giant should feel like. An infinite weight would need a special case in `SeparateCircles`, and nothing in the fight needs it.
- **Slower than a hairball when it walks.** The walk is only how the boss gets close between attacks. If the walk alone could catch the player, the attacks would not matter.
- **The camera leans toward the boss, with a cap.** In a fight with one enemy, losing sight of it is the worst thing the camera can do. Leaning 40% of the way keeps both on screen at the start and in most of the fight. The cap of 200 units keeps the player at least 160 units from the edge of the screen when the boss is across the arena.
  - Rejected: zooming out to fit both. It changes the scale of everything, so the dash and the guns would look shorter or longer depending on where the boss is.
  - Rejected: following the player only. With the boss off screen, the player cannot read its attacks.
- **The boss's hit flash is only part of the way to white.** The SMG would keep a full white flash on for most of the fight. At 45% the hit still shows, and the boss keeps its colour.
- **The health bar is at the bottom, wide and thin.** It reads as the one fight that matters and stays away from the player's own bars in the top left corner.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Health | `Boss::health` (`src/core/Boss.h`) | 1200: 120 pistol or 200 SMG bullets, while the shield is down |
| Shield and the stagger of a break | `Boss::shield`, `breakStaggerSeconds` | See [The boss's shield](SHIELD.md#how-to-tune-it) |
| Contact damage | `Boss::contactDamage` | 20 |
| Walking speed | `Boss::stats.maxSpeed` | 120 units/s |
| Acceleration and deceleration | `Boss::stats` | 400 and 1500 units/s^2 |
| Body radius | `Boss::radius` | 60 units |
| Weight | `Boss::weight` | 40 (player 4, hairball 1) |
| First stalk | `Boss::secondsUntilAttack` | 2 s |
| Stalks after that | `AttackChoiceStats::minStalkSeconds`, `maxStalkSeconds` (`src/core/AttackChoice.h`) | 0.8 to 1.6 s |
| First attack | `Boss::nextAttack` | The Charge |
| Weights by distance | `AttackChoiceStats::charge`, `hairRing`, `leap` (`AttackWeights`) | Charge 1, 2, 4; Ring 4, 2, 1; Leap 1, 4, 2 |
| Near and far distance | `AttackChoiceStats::nearDistance`, `farDistance` | 250 and 550 units; the middle weights at 400 |
| Random stretch of every warning | `AttackChoiceStats::minWarningScale`, `maxWarningScale` | 0.85 to 1.15 |
| Combo chance | `ComboStats::chance` | 35% |
| Follow-up warning, and the recovery after it | `ComboStats::warningScale`, `finalRecoverySeconds` | 0.7, at least 1.2 s |
| The combos | `kCombos` (`src/core/AttackChoice.h`) | Rebound, Pursuit, Crash |
| Volley warning, bursts, time between them, active and recovery | `VolleyStats::warningSeconds`, `bursts`, `burstIntervalSeconds`, `activeSeconds`, `recoverySeconds` (`src/core/Boss.h`) | 0.55 s, 3, 0.3 s, 0.15 s, 0.8 s |
| Volley strands: per burst, spread, speed, range, radius, damage | `VolleyStats::strandsPerBurst`, `spreadDegrees`, `speed`, `range`, `strandRadius`, `damage` | 5, 8 degrees, 520 units/s, 900 units, 7 units, 12 |
| How far ahead the led burst looks, at most | `VolleyStats::maxLeadSeconds` | 1.5 s |
| Transformation | `PhaseTwoStats::transformSeconds` | 2 s |
| Second phase: weights, stalks and warnings | `PhaseTwoChoice` (`src/core/Boss.h`) | Volley 1, 3, 3; stalks 0.6 to 1.2 s; warnings 0.75 to 1.0 |
| Second phase: combos and exposed time | `PhaseTwoStats::combo`, `exposedSeconds` | 50%, 1 s of rest after; 4 s |
| The roar's shockwave | `PhaseTwoStats::roar` | 380 units/s, 300 units, 20 units, 20 |
| When the frenzy starts | `FrenzyStats::healthFraction` | Half of the second bar |
| The frenzy's extras | `FrenzyStats` (`chargeBoltIntervalSeconds`, `ringWaves`, `ringGapTurnDegrees`, `leapShadowSpeed`, `leapLandingStrands`, `volleyBursts`, `volleyBurstIntervalSeconds`) | 0.15 s, 2, 30 degrees, 240 units/s, 12, 5, 0.25 s |
| Summon warning, active and recovery | `SummonStats::warningSeconds`, `activeSeconds`, `recoverySeconds` (`src/core/Boss.h`) | 0.75 s, 0.2 s, 1 s |
| From the marks to the hairballs, and the most alive at once | `SummonStats::markerSeconds`, `maxAlive` | 0.75 s, 8 |
| Marks: how many, how far, inside the walls, and how far from the player and the boss | `SummonPlacement::count`, `distance`, `wallMargin`, `minPlayerDistance`, `minBossDistance` (`src/core/SpawnMarker.h`) | 4 (6 in a frenzy), 250, 40, 150, 100 units |
| Summon weights in the second phase | `PhaseTwoChoice` | 0.5, 1, 2.5 |
| Hairball spit: interval, first one, warning | `SummonStats::spitIntervalSeconds`, `firstSpitMinSeconds`, `firstSpitMaxSeconds`, `spitWarningSeconds` | 3 s; 1 to 3 s after it comes out; 0.4 s |
| Hairball spit: speed, range, radius, damage | `SummonStats::spitSpeed`, `spitRange`, `spitRadius`, `spitDamage` | 300 units/s, 700 units, 6 units, 8 |
| Marks at once at most | `kMaxSpawnMarkers` (`src/core/World.h`) | 16 |
| The frenzy's rhythm and roar | `FrenzyChoice`, `FrenzyStats::combo`, `roar` | Stalks 0.4 to 0.9 s; 65%, 0.9 s of rest; the same roar as the second phase's |
| Charge warning, and its locked part | `ChargeStats::warningSeconds`, `aimLockSeconds` | 0.8 s, the last 0.3 s |
| Charge speed and distance | `ChargeStats::speed`, `distance` | 1000 units/s, 800 units |
| Charge damage and knockback | `ChargeStats::damage`, `knockback` | 30, 700 units/s |
| Recovery in the open, stun at a wall | `ChargeStats::recoverySeconds`, `stunSeconds` | 0.5 s, 1.5 s |
| Hair Ring warning, spit and recovery | `HairRingStats::warningSeconds`, `activeSeconds`, `recoverySeconds` | 0.6 s, 0.15 s, 0.8 s |
| Directions and gap | `HairRingStats::slots`, `gapSlots` | 48 and 6: 42 strands, a 45 degree gap |
| Where the gap opens | `HairRingStats::gapAngleDegrees` | 90 degrees from the player, on alternate sides |
| Strand speed, range, radius and damage | `HairRingStats::speed`, `range`, `strandRadius`, `damage` | 220 units/s, 900 units, 8 units, 15 |
| Rings per attack, time between them, turn of the gap | `HairRingStats::waves`, `waveIntervalSeconds`, `gapTurnDegrees` | 1, 0.4 s, 0 degrees |
| Strands in flight at most | `kMaxHostileProjectiles` (`src/core/World.h`) | 128, three rings |
| Leap crouch, air time, and the locked part of it | `LeapStats::crouchSeconds`, `airSeconds`, `lockSeconds` | 0.35 s, 1 s, the last 0.35 s |
| Shadow speed | `LeapStats::shadowSpeed` | 200 units/s |
| Landing zone, impact damage and knockback | `LeapStats::impactRadius`, `impactDamage`, `impactKnockback` | 100 units, 35, 600 units/s |
| Landing and recovery | `LeapStats::landingSeconds`, `recoverySeconds` | 0.15 s, 1 s |
| Shockwave speed, reach, band and damage | `LeapStats::shockwave` (`ShockwaveStats`, `src/core/Shockwave.h`) | 380 units/s, 420 units, 20 units, 20 |
| Shockwaves at once at most | `kMaxShockwaves` (`src/core/World.h`) | 8 |
| Arena | `kBossArena` (`src/core/World.h`) | 1640x1040 |
| Starting distance | `kStartingDistance` (`src/core/World.h`) | 400 units |
| Camera lean and its cap | `kBossFraming`, `kMaxBossFramingOffset` (`src/core/World.cpp`) | 40%, 200 units |
| Look | `kBossStyle`, `kBossBar`, the boss bar constants and the `kCharge...`, `kStun...`, `kRing...`, `kStrand...`, `kLeap...` and `kShockwave` constants (`src/app/Game.cpp`) | |

In a Debug build, `F1` shows the boss's health next to the shots fired and hit, the strands in flight (`hair`), the shockwaves (`waves`), and what the boss is doing: stalking and the time to its next attack, or the attack, its stage, its clock against the warning's length this time, whether it is in the air, and `combo 1/2` or `combo 2/2` inside a combo. The `pick` line shows the distance to the player, the weight of each attack at that distance, and the attack the next pick skips. Its first word is the phase (`p1` or `p2`), or `transforming` with the time left. `F3` starts the transformation at once (`TransformBoss`), to skip the first phase. `F4` defeats the boss at once (`DefeatBoss`), to reach the victory screen without shooting.

## Tests

The tests about one attack's timings use a boss without its chance (`tests/core/PredictableBoss.h`): every warning at its stats' exact length and no combos.

- `tests/core/BossVolleyTests.cpp`:
  - during the Volley's warning the boss holds still and its aim follows the player;
  - a Volley fires three bursts a beat apart, each aimed where a standing player is when it leaves;
  - the second burst leads a running player, and its middle strand meets them; the first and the third aim at them;
  - a led burst looks ahead at most its cap, and never past the walls;
  - a player who runs in a straight line through a whole Volley is hit, running across its aim, away from the boss, on a diagonal or close by;
  - after the last burst the boss recovers, then stalks again;
  - a burst is five strands fanned 8 degrees apart around the aim, leaving the edge of the body at the Volley's speed;
  - a Volley strand that hits the player counts as the Volley's.
- `tests/core/SummonTests.cpp`:
  - in the open, a summon's points ring the player at its distance, evenly spaced, turned by the angle given;
  - by a wall or in a corner, a point pushed inside the walls onto the player is dropped;
  - a point that would put a hairball inside the boss is dropped;
  - a spawn marker waits its time, then lets go;
  - a Summon puts its marks down as the howl starts, then holds still, recovers and stalks;
  - each mark lets out a hairball where it was, which shakes itself, then chases the player;
  - a Summon brings fewer hairballs when the arena already holds as many as the boss allows;
  - in a frenzy a Summon brings six;
  - summoned hairballs spit, each on a beat of its own;
  - a spitting hairball sends a strand at the player, which hits for 8 as the hairball's, then waits its 3 s for the next;
  - a hairball the boss did not summon never spits.
- `tests/core/FrenzyTests.cpp`:
  - the bullet that takes the second phase's bar to half throws the boss into a frenzy, with a roar, and only once;
  - the first phase never falls into a frenzy;
  - a frenzied Charge throws a pair of strands on a beat while it runs (six pairs over a 760 unit run);
  - a frenzied Hair Ring spits two rings;
  - a frenzied Leap lands with a ring of 12 strands with no gap, one of them at the player;
  - a frenzied Volley fires five bursts, led every other one;
  - the Debug shortcut throws the second phase into its frenzy, and does nothing in the first.
- `tests/core/PhaseTwoTests.cpp`:
  - emptying the first phase's bar starts the transformation, not the victory;
  - while it transforms the boss cannot be hurt, holds still and does not attack;
  - the transformation ends with a roar, a fresh shield and the second phase's numbers, and the Volley comes first;
  - the roar's shockwave catches a player who waits next to the boss, and dies out before the distance the fight starts at;
  - only an empty bar in the second phase wins the fight;
  - the Debug shortcuts skip to the transformation (only once) and to the victory;
  - the Volley joins the picks in the second phase (three in ten far away), and never shows in the first.

- `tests/core/AttackChoiceTests.cpp`, on the choice alone:
  - each attack weighs most at its own distance, and the weights blend in between;
  - the attack that just ended is never picked again;
  - the distance shifts the picks toward the attack that suits it, over 1000 picks;
  - the same seed picks the same attacks;
  - a charge or a leap opens a combo about a third of the time, and a hair ring never;
  - a combo chance of 0 never chains, and of 1 always does.
- `tests/core/BossComboTests.cpp`, on the boss alone, with its chance:
  - a fight opens with a charge, then never repeats an attack outside a combo, over five seeds and 90 s each;
  - the stalks and the warnings vary within their ranges;
  - combos happen in a fight, and only the known ones;
  - the same seed plays the same fight, and another seed does not;
  - in a combo the second attack starts in place of the first one's recovery, with a shorter warning (the Rebound, off the wall);
  - the last attack of a combo rests for the combo's longer recovery (the Crash);
  - a longer or shorter warning keeps the charge's aim lock at its exact length;
  - a shorter leap keeps the shadow's lock, and the boss is in the air after its shorter crouch.
- `tests/core/BossChargeTests.cpp`, on the boss alone, without a world:
  - the boss stalks the player for two seconds, then opens the fight with a charge;
  - during the warning it stands still, and its aim follows the player until it locks;
  - the charge runs in a straight line at its speed;
  - a charge into a wall stops at the wall and stuns the boss for the long recovery, then the boss stalks again for a stalk in its range;
  - a charge that runs out in the open stops after its distance and recovers quickly;
  - a charge path runs its full distance in the open and stops at the first wall, also on a diagonal;
  - a dead boss drops its attack and stays where it fell.
- `tests/core/BossHairRingTests.cpp`, on the boss alone:
  - during the warning the boss holds its ground, and the gap opens 90 degrees from the player and stays there when the player moves;
  - the gap opens on one side of the player, and on the other side the next time;
  - the ring is spat once, as the warning ends, and the boss stalks again after the recovery;
  - a ring has a strand in every slot but the gap, evenly spaced, and none inside the gap;
  - a strand leaves the edge of the body and flies at the ring's speed for its range;
  - with more waves, each ring leaves after the wave interval and turns the gap.
- `tests/core/BossLeapTests.cpp`, on the boss alone:
  - the leap crouches on the ground, then takes off;
  - the shadow appears under the player, follows them at its speed, then locks;
  - the boss flies to its shadow and lands on it, once;
  - the shadow stays where the whole body fits inside the arena;
  - after landing the boss recovers for a second, then stalks again.
- `tests/core/ShockwaveTests.cpp`: a shockwave grows at its speed and dies out past its reach; only the band of the ring touches, not inside it and not outside it.
- `tests/core/BossTests.cpp`, in a world, where every hit also checks that `World::lastHitBy` names the attack that landed it:
  - the boss walks toward the player, slower than a hairball;
  - it stops when it touches the player, and pushes the player rather than being pushed;
  - a touch takes its contact damage, and when the boss and a hairball touch the player in the same step, the boss's hit is the one that lands;
  - with its shield down, a bullet takes health from the boss and flashes it, but does not push it; the shield's own tests are in [The boss's shield](SHIELD.md#tests);
  - a hairball in front of the boss takes the bullet meant for the boss;
  - a dash goes through the boss without a scratch;
  - a dead boss does not chase, block, take bullets or hurt;
  - a charge that catches the player takes the charge's damage and throws them out of its path;
  - the charge runs through the player instead of carrying them along, and ends stunned at the wall;
  - a dash through the charge takes no damage;
  - the boss counts as charging only while it runs;
  - the strands that reach the player take one strand's damage, and are spent even inside the window;
  - standing in the gap, the whole ring flies past;
  - a dash through the ring takes no damage, and the strands fly on;
  - the strands hurt only the player: hairballs, the boss and bullets let them through;
  - a strand that reaches a wall is gone at once;
  - landing on the player takes the impact's damage once and throws them out from under the boss;
  - in the air the boss is not a body: it does not block, take bullets or hurt;
  - the shockwave hurts a player outside the landing zone once, when its band arrives;
  - a dash through the shockwave takes no damage;
  - beyond the shockwave's reach the landing is harmless;
  - running from the takeoff escapes the landing, but starting only when the shadow locks does not;
  - a new fight starts with the camera settled between the player and the boss, with both in view;
  - the camera keeps a far boss in mind without pushing the player off screen;
  - once the boss is dead, the camera goes back to the player alone.
- `tests/core/WorldTests.cpp`: a new world has the boss in the middle and the player south of it, facing it; nobody leaves the arena, the boss included; and two worlds with the boss, hairballs and the same inputs end in exactly the same state, strands included.
- `tests/core/SessionTests.cpp`: standing still in front of the boss ends in death for each of 20 seeds, before 20 s (between 6.3 and 18.8 s, most often to the boss's touch once a charge has left it next to the player); a dead player's world remembers what killed them; and the last bullet into the boss wins the fight in the step it lands.

The drawing is not unit tested, because it needs a window. I checked it in a Release build with captures that never touch the real keyboard or mouse. For the Charge, standing still from the start: the path following the player, the brighter locked path with the boss crouched, the ghosts behind the running boss, the stun stars with the player's health at 70, and the "YOU DIED" screen at 0:07. For the Hair Ring, standing still dies before the first ring, so I drove the player with key presses posted to the game's window: a sidestep out of the Charge, a walk north, then a run into the gap. The captures show the swollen boss with the red ring and the pale lane, the ring leaving the boss, and the ring passing the player in the gap at full health. A first run that stopped at the edge of the gap took 15. For the Leap, the same run went on: the shadow under the player, the boss in the air with the shadow lagging behind the running player, the landing next to the player at 60 health, and a dash through the shockwave that left the player at 60 and the wave unspent. Runs that started running too late took the impact (60 to 25), and a dash pressed too late took the wave (60 to 40). In a Debug build, `F1` read "ring warning" during the warning and 42 strands in flight after the spit, and "leap warning 0.90 airborne" with the seed during a Leap. For the random choice and the combos, every launch has a new seed, so I ran the game ten times in Debug and nine in Release with the same sidestep out of the opening Charge, and captured it just after the run. Three of the nineteen chained a combo (the tests measure the 35% over thousands of picks; nineteen runs are too few to show it): in Debug, `F1` read "charge warning 0.52/0.54 combo 2/2" with the Rebound's path drawn from the south wall back at the player; in Release, the Pursuit had the boss in the air over the player's shadow at 3.75 s, where a lone Charge would still be stunned. The Debug runs without a combo read "charge recovery" and "stunned", with warnings between 0.71 and 0.91 s, all inside the 0.68 to 0.92 s range. For the second phase, in a Debug build, `F3` at 1 s: at 1.5 s the swollen boss with red eyes and "transforming 1.45" over a bar a quarter full; at 3.2 s the roar's shockwave around the boss, the red eyes and a fresh Shell; at 3.9 s "p2 volley warning" with the line on the floor to the player; then the three strands of each burst, and the player, standing still, at 68 after a burst's 12. In that run the boss had walked to about 330 units during its first stalk, and the roar's last steps reached the player for 20; a first version with the landing's 420 unit reach hit the player even at the starting 400, which is why the roar is shorter. For the Summon, which only comes now and then, I ran the second phase ten times in Debug with the player running to the south-west and back, and caught one: four marks with closing red rims around where the player stood, `F1` reading "p2 summon warning 0.10/0.64" and "marks 4" (the player was 377 units away, where the Summon weighs 0.9 against the Leap's 3.5: it was a lucky pick for the capture), then four hairballs out and closing in, one of them taking 10 off the player. For the hairballs' spit, six more runs: in one, three hairballs alive, one of them swollen with red eyes next to the player just before it spat, then strands leaving the hairballs (`F1` read "hair 2" with no attack of the boss in flight). For the frenzy, `F3` twice (the transformation, then the frenzy at 3.3 s): the second roar around the bristling, reddened boss with its glow; a frenzied Volley of five bursts of five strands; then a Rebound ("p2 frenzy charge active 0.65/0.46 combo 2/2") with pairs of strands left along its run, one at the player and one ahead. For the led burst, the same start with the player running west, away from the boss, through the whole Volley: the first burst passed behind the player and the second, led, hit at 5.3 s (80 to 68). Before the lead, at 400 units/s, the same run ended at 80. Dodging with the dash, punishing the recovery and how the timing feels need someone playing, so I check them by playing.

## Limitations

- **The pick looks only at the distance.** It does not know whether the player is drinking, out of dash, or standing in a corner. A boss that punishes a drink is a good idea for the second phase, and it would be one more weight, not a new system.
- **The numbers are a first guess.** The weights, the combo chance and the stalks are set from the captures and the tests, not from people playing. Day 9 moves them to a data file that reloads with `F5`, to tune them while playing.
- **Hairballs do not avoid the boss's attacks.** A Charge bowls them aside and the boss's strands fly through them, so a crowd can never shield the player, and the boss never hurts its own children.
- **The transformation and the second phase are only checked in Debug.** Reaching the second phase in a Release build takes emptying the first bar through its shields, which the captures cannot do. I used `F3` in a Debug build.
- **The Leap's arc is drawn, not simulated.** The body's position moves straight from the takeoff to the shadow, and the height is only an offset in the drawing. Nothing in the fight needs a third dimension.
- **The crouch and the swell change the size, not the shape.** The body is drawn with circles, so it gets smaller or bigger instead of flatter or rounder. Squash and stretch come with the art pass.
- **A strand is a dot with a short tail.** Real hair, and strands that fall to the floor at the end of their range, come with the art pass.
- **At the south wall the boss is partly hidden by the HUD.** The boss bar and the controls line cover the bottom of the screen, where the camera stops for the south wall. The art pass will rework the HUD.
- **The bar has no animation.** It jumps to the new value. A trailing chunk that drains behind it is part of the game feel pass.
