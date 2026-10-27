# The boss's shield

## What it is

The shield the Matriarch wears between the bullets and her health. It comes in two kinds that take turns, and each one only gives way to one of the two guns, so the fight asks the player to switch guns in the middle of the boss's attacks. Breaking a shield staggers the boss and leaves it open to every bullet for a few seconds; then the other kind comes up.

The rules live in `horde_core`:

- `src/core/Shield.h`, `src/core/Shield.cpp`: the `Shield` component (`ShieldKind`, `ShieldStats`), `HitShield`, which applies one bullet and says what it did (`ShieldHit`), `StepShield`, `BreakShield`, `MaxStrength` and `IsShieldUp`.
- `src/core/Boss.h`, `src/core/Boss.cpp`: the boss carries a `Shield`; `StepBoss` steps it, and `StaggerBoss` and `IsStaggered` handle the stagger of a break.
- `src/core/World.cpp`: `HitBoss` sends every bullet that reaches the boss to its shield first; `BreakBossShield` is the Debug shortcut.

The game executable draws it:

- `src/app/render/BossDraw.cpp` and `Hud.cpp`: `DrawBossShield` draws the band around the body (through `DrawBossBody`, so it follows every pose of the boss), and `DrawShieldBar` the bar under the boss's health bar, with the shield's name at its right end.

## How it works

```
  Shell (blue)  -- broken by the pistol -->  Exposed, 5 s  -->  Barrier (orange)
       ^                                                             |
       +------------------  Exposed, 5 s  <-- broken by the SMG -----+
```

**The two kinds.** The fight opens with the Shell, which the pistol in hand breaks.

| Kind | Strength | Rule | Pistol (10 x 6/s) | SMG (6 x 10/s) |
|---|---|---|---|---|
| Shell, blue | 150 | A bullet under 8 damage bounces off and takes nothing. | Takes 10 a bullet: broken in 2.5 s of steady fire. | Every bullet bounces. |
| Barrier, orange | 100 | A bullet takes at most 6. It grows back at 40 a second while it is up. | 6 a bullet, 36 a second: less than it grows back, so it never breaks. | 60 a second against 40: 20 net, broken in 5 s. |

**One bullet.** `HitBoss` calls `HitShield` with the bullet's damage:

- `Bounced`: the Shell shrugged it off. The band flashes thicker.
- `Absorbed`: the bullet took strength off the shield. The band flashes white.
- `Broken`: the bullet took the last of it. The shield goes down for `exposedSeconds` (5 s), and `StaggerBoss` cuts the boss's attack off and holds it still for 1 s, with stars over its head. After the stagger it stalks for the shortest stalk (0.8 s) before its next attack.
- `Exposed`: there was no shield. The bullet hits the boss's health, as it did before the shield existed.

The bullet is spent in every case: a bounce does not fly on. Every bullet that reaches the boss counts as a hit for the accuracy on the victory screen, bounced or not.

**Every step,** `StepShield` fades the flashes, grows the Barrier back while it is up, and counts the exposed time down. When it runs out, the other kind comes up at full strength.

**On screen.** The band sits just outside the tips of the tufts, over a faint bubble in its colour. It is brighter the more strength is left, and the Barrier's band breathes in and out. Under the boss's health bar, a thin bar shows what is left of the shield in its colour, with "SHELL" or "BARRIER" above the right end. Once it breaks, the bar turns white, reads "EXPOSED", and drains with the exposed time.

## Design decisions

- **Each shield has a rule that is a number, not a list of guns.** The Shell ignores small hits and the Barrier caps every hit and grows back, so it rewards a high rate of fire. A new gun falls on one side or the other by its damage per bullet and its fire rate, with no code that asks which gun fired.
- **The Barrier caps each hit, as well as growing back.** The first plan was a Barrier that only grows back (about 40 a second) and that only the SMG could outpace. But at full rate the pistol also does 60 a second (10 x 6), the same as the SMG (6 x 10), so a player clicking fast enough could break it with either gun. Capping each hit at 6 makes the pistol worth 36 a second against it, below the 40 it grows back, while the SMG loses nothing.
  - Rejected: a faster regeneration (say 65 a second). It would also beat the SMG, and the numbers would have to change every time a gun's fire rate does.
  - Rejected: "only bullets from an automatic gun count". It is a check of which gun fired, the thing I wanted to avoid.
- **The fight opens with the Shell.** The player starts with the pistol in hand, so the first shield is the one they can already break, and the first switch comes after the first break, when the player has learned what a break looks like.
- **A broken shield staggers the boss.** A break is the player's reward, and a bar that empties is easy to miss in the middle of a dodge. Cutting the attack off and holding the boss still for a second makes the break felt, and it is the one moment a bullet can interrupt the boss. Bullets alone still never stagger it (see [The boss](BOSS.md#design-decisions)).
- **Exposed for 5 s, then the other kind.** Long enough for about 300 damage with either gun, a quarter of the boss's health, so a fight is a few shield cycles. Coming back as the other kind is what forces the switch; coming back as the same kind would let the player keep one gun for the whole fight.
- **The shield is a component of the boss, stepped by `StepBoss`.** It has its own data and rules and its own tests, but it only exists on the boss, so it lives in the `Boss` struct like the attack stats, not in the `World`.
- **Bounced bullets are spent.** A bullet that flies back off the shield would look good, but it would need a new side (a bullet that can hit the player, or nothing) and more drawing for little gain. The thicker flash of the band and a tight spray of blue ricochet sparks say "bounced" clearly without a new projectile rule.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Shell strength, and the smallest hit it takes | `ShieldStats::shellStrength`, `shellMinimumDamage` (`src/core/Shield.h`) | 150, 8 |
| Barrier strength, the most one hit takes, and how fast it grows back | `ShieldStats::barrierStrength`, `barrierMaximumDamage`, `barrierRegeneration` | 100, 6, 40 a second |
| Exposed time | `ShieldStats::exposedSeconds` | 5 s |
| Flash of a hit or a bounce | `ShieldStats::flashSeconds` | 0.1 s |
| First shield | `Shield::kind` | The Shell |
| Stagger of a break | `Boss::breakStaggerSeconds` (`src/core/Boss.h`) | 1 s |
| Look | `kShell`, `kBarrier`, `kExposed`, `kShieldGap`, `kShieldWidth`, `kShieldBounceWidth`, `kBarrierPulse`, `kShieldBarHeight` (`src/app/render/Palette.h`, `BossDraw.cpp`, `Hud.cpp`) | |

In a Debug build, `F2` breaks the shield at once and staggers the boss, like a real break (`BreakBossShield`), and the last line of the `F1` overlay shows the shield's kind and strength, or the exposed time and the stagger left.

## Tests

- `tests/core/ShieldTests.cpp`:
  - the Shell bounces bullets under its minimum damage, and takes the rest in full, without growing back;
  - the Barrier takes at most its maximum per bullet, and grows back to full but not past it;
  - a broken shield leaves the boss exposed for 5 s, every bullet goes through meanwhile, then the other kind comes up at full strength, and after the Barrier the Shell again;
  - only the pistol breaks the Shell (in 2.33 s of steady fire: 15 bullets, the first at 0 s) and only the SMG breaks the Barrier (in 4 to 6 s), each gun firing at its full rate for 30 s.
- `tests/core/BossTests.cpp`, in a world:
  - a shield that is up takes the bullets instead of the boss, and the shot still counts as a hit;
  - SMG bullets bounce off the Shell without a scratch;
  - the bullet that breaks the shield staggers the boss: its Charge is cut off, it holds still for a second, then stalks again;
  - the Debug shortcut breaks the shield and staggers the boss like a real break;
  - with its shield down, a bullet takes health from the boss and flashes it, but does not push it.
- `tests/core/SessionTests.cpp`: the fight is won in the step the last bullet lands, with the shield down.

The drawing is not unit tested, because it needs a window. I checked it with captures. In a Release build, with left clicks posted to the game's window every 0.175 s at the boss: the blue Shell with its full bar at the start, the band flashing white and the bar at about half at 1.8 s, then at 3.5 s "EXPOSED" with the white bar draining, stars over the boss and the first bullets taking its health. In a Debug build, `F2` at 1 s gave "shield exposed 4.72 stagger 0.72", and at 6.3 s the orange Barrier was up, reading "shield barrier 100 / 100". Breaking the Barrier with the SMG in the game, switching guns under pressure, and whether 5 s feel like enough, I check by playing.

## Limitations

- **A bounce has no sound yet.** The band's flash and the blue ricochet sparks show it; sound comes with the audio pass.
- **The shield ignores the hairballs.** It only stands between the player's bullets and the boss. Nothing else hurts the boss today.
- **Only the opening changes in the second phase.** It drops from 5 s to 4 s (`PhaseTwoStats::exposedSeconds`), and the second phase starts with a fresh Shell. The strengths stay the same.
