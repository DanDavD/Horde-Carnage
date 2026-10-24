# Collisions

## What it is

How bodies in the arena push each other: the player, the boss and the hairballs. Every body is a circle. When two circles overlap, they are pushed apart, and the heavier one moves less. This is what keeps a crowd of hairballs from stacking into one, what lets the player shove through a single hairball but get slowed down by a crowd, and what lets the boss push the player around.

The rules live in `horde_core`:

- `src/core/Collision.h`, `src/core/Collision.cpp`: `SeparateCircles` resolves one pair, `PushShare` turns two weights into how the push is split, and `kContactSlack` says how close counts as touching. `SweepCircle`, the hit test for bullets, lives in the same file and is described in [Bullets and hits](BULLETS.md).
- `src/core/World.cpp`: `StepWorld` runs the collision passes over every pair of bodies, with the walls after each pass.
- `src/core/Player.h`, `src/core/Boss.h` and `src/core/Enemy.h`: each body has a `weight` next to its `radius`.

## How it works

Every fixed step, `StepWorld` first moves everyone (the player with its input, the hairballs with their chase input, and the boss as it stalks or attacks). Only then does it resolve collisions, in 4 passes. Each pass:

1. **Checks every pair once:** the player against the boss, the player and the boss against each hairball, and each hairball against every hairball after it in the list. A dashing player is left out, so a dash passes through bodies (see [Player abilities](ABILITIES.md)). A dead boss is left out too, and so is the pair of the player and a charging boss: at the charge's speed a shove would carry the player across the arena, so the run goes through them instead (see [The boss](BOSS.md)).

2. **Separates each overlapping pair** with `SeparateCircles`:
   1. If the distance between the centres is less than the sum of the radii, the circles overlap by the difference.
   2. Both bodies move apart along the line between their centres until they just touch. Body A moves `PushShare(weightA, weightB) = weightB / (weightA + weightB)` of the overlap, body B the rest. With the player at weight 4 and a hairball at 1, the player takes 20% and the hairball 80%. Against the boss at 40, the player takes 91%.
   3. The speed at which they were closing in on each other is removed, split by the same shares. This is a perfectly inelastic collision: afterwards both move at the same speed along that line. Speed across the line is kept, so bodies slide past each other.
3. **Holds everyone inside the arena** with `ConfineToArena`, so a body pushed into a wall is put back before the next pass.

Chasers stop pushing when they are within `kContactSlack` (1 unit) of touching the player. Collisions leave touching bodies at exactly the sum of their radii, give or take float rounding, and without the slack a hairball resting against the player would flicker between walking and stopping.

## Design decisions

- **Circles only.** Every body in the game is round, and circle against circle is one distance check.
  - Rejected: boxes or polygons (separating axis test). Nothing in the game needs them yet.
- **Resolve collisions after everyone has moved, not as each body moves.** Otherwise the body that moved first would always be the one that gets pushed, and the result would depend on the order of the enemy list.
- **Push positions apart and remove the closing speed.** Moving the positions alone is not enough: a hairball in the second row of a crowd would keep its full speed into the first row and push again every step. Removing the closing speed is the same idea as `ConfineToArena` cancelling the velocity into a wall. A wall is a body of infinite weight.
  - Rejected: a physics engine such as Box2D. It is a large dependency for circles that push each other, its bouncy rigid bodies are not the feel I want, and it would hide the part of the code I want to show.
  - Rejected: bouncing (an elastic collision). A hairball that bounces off the player looks like a ball, not like a lump of hair.
- **Weights instead of "enemies always yield".** I want one hairball unable to block the player, but a crowd able to slow them down, because being surrounded should be dangerous. In a test, pushing a column of hairballs straight ahead for a second, the player moved at:

  | Hairballs in the column | Player speed (top speed 300) |
  |---|---|
  | 1 | 292 |
  | 3 | 271 |
  | 5 | 213 |
  | 8 | 51 |

  - Rejected: enemies that never push the player. The player would walk through any crowd at full speed, and getting surrounded would mean nothing until contact damage.
- **Several passes.** Pushing one pair apart can push one of them into a third body, so one pass leaves overlaps inside a crowd. I picked 4 as a starting point. The tests check that a crowd that followed the player ends with less than 0.5 units of overlap, and that bodies resting around the player do not overlap at all.
- **Every pair, every pass.** With the player, the boss and 6 hairballs that is 28 pairs, 112 checks per step. With 50 enemies it would be about 5,000 checks per step, which is still cheap.
  - Rejected for now: a spatial grid. It only pays off with hundreds of bodies, and it adds code and bugs to a step that is already fast.
- **Two bodies on exactly the same point are pushed apart along x.** There is no line between their centres. A fixed direction keeps the simulation deterministic, which the determinism test in `WorldTests` relies on.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Player weight | `Player::weight` (`src/core/Player.h`) | 4 |
| Hairball weight | `Enemy::weight` (`src/core/Enemy.h`) | 1 |
| Boss weight | `Boss::weight` (`src/core/Boss.h`) | 40 |
| Collision passes per step | `kCollisionPasses` (`src/core/World.cpp`) | 4 |
| Touching slack | `kContactSlack` (`src/core/Collision.h`) | 1 unit |

Only the ratio between two weights matters. A higher player weight lets the player shove through bigger crowds.

## Tests

- `tests/core/CollisionTests.cpp`: the share of the push for equal and unequal weights; bodies apart and bodies that just touch are left alone; equal weights split an overlap in half; the heavier body moves less; two bodies on the same point separate along x; a heavy body running into a still one ends at the same speed as it (240 for weights 4 and 1 at 300); bodies already moving apart keep their speed; and speed across the contact is kept.
- `tests/core/WorldTests.cpp`: hairballs come to rest touching the player without sinking into it or each other; hairballs that follow a running player spread out instead of piling up; a crowd leans on a standing player at under 2% of the player's top speed; and the player can shove through a hairball, slower than running free.
- `tests/core/BossTests.cpp`: the boss stops when it touches the player, and pushes the player rather than being pushed (a second of the player running into it moves it less than 15 units); and a dead boss does not block.

I also played it by hand in a Release build: being surrounded, running away and stopping without the hairballs piling up, and shoving through a single hairball.

## Limitations

- **A crowd leans on a standing player.** Hairballs in the second row cannot touch the player, so they keep walking into the first row, and a little of that push reaches the player. In a test where all six hairballs followed the player and then crowded behind it, it moved 0.5 to 3 units per second, slowing down over time, against a top speed of 300. Removing it completely would need hairballs that queue behind each other, and then they would stop flowing around the player to surround them.
- **A body against a wall is not treated as heavier.** The passes put it back inside, but in a corner full of hairballs some overlap may remain. I have not measured it yet.
- **No continuous collision.** A body moves up to 5 units per step at top speed and the smallest radius is 18, so nothing can pass through another body in one step. A much faster body (a dash, a projectile) would need a swept test.
