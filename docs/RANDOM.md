# Random numbers

## What it is

The seeded random number generator that everything random in the fight draws from. A fight is decided by its seed and the player's inputs alone: the same seed and the same inputs always play out the same way. That keeps the tests deterministic, and it makes a bug seen in one fight reproducible.

Today the boss is its only user: which attack comes next, how long each stalk and each warning lasts, and when two attacks chain into a combo (see [The boss](BOSS.md#how-it-works)).

The rules live in `horde_core`:

- `src/core/Random.h`, `src/core/Random.cpp`: the `Random` struct and its functions, `SeedRandom`, `NextU32`, `NextFloat`, `RandomRange` and `Chance`.
- `src/core/World.h`, `src/core/World.cpp`: every `World` has its own `Random`, seeded by `CreateWorld`, and keeps the `seed` it started from.
- `src/core/Session.h`, `src/core/Session.cpp`: the session hands every new world its seed.
- `src/core/Boss.cpp`, `src/core/AttackChoice.cpp`: the boss draws from the world's generator, which `StepWorld` passes to `StepBoss`.

The game executable picks the session's seed from the system (`std::random_device`) at launch, and in a Debug build `F1` shows the seed of the current fight.

## How it works

**The generator** is PCG32 by Melissa O'Neill (pcg-random.org), the XSH RR variant: 64 bits of state and an odd increment that selects the stream. Each `NextU32` advances the state with one multiply and one add (a linear congruential step), then scrambles the old state into 32 bits: an xorshift of the high bits, then a rotation chosen by the top five bits. `SeedRandom(seed, stream)` sets it up exactly like the reference `pcg32_srandom_r`, so it gives the reference outputs.

**On top of the 32 bits:**

- `NextFloat` takes the top 24 bits and scales them to [0, 1). A float has a 24 bit significand, so every value is exact and evenly spaced.
- `RandomRange(min, max)` scales that to [min, max).
- `Chance(probability)` is `NextFloat() < probability`: never at 0, always at 1.

**Seeds:**

```
  system seed --> Session::seeds --> seed of attempt 1 --> World::random
                                 --> seed of attempt 2 --> World::random
                                 --> ...
```

- `StartSession(arena, view, seed)` seeds the session's own generator, `Session::seeds`, on a stream of its own, and draws the first world's seed from it.
- `TryRestart` draws the next one, so every attempt plays out differently, and the whole run of attempts follows from the one seed the session started with.
- `CreateWorld(arena, view, seed)` seeds `World::random` and stores `World::seed`. Tests that do not depend on chance leave the default seed, 0.

## Design decisions

- **PCG32, written by hand.** It is small (two 64 bit numbers of state, a few lines of code), fast, and statistically far better than a plain linear congruential generator. Writing it myself means its output is the same on every compiler and standard library, which a deterministic simulation needs.
  - Rejected: `std::mt19937`. Its output is specified by the standard, but the distributions on top of it (`std::uniform_real_distribution` and the rest) are not, so the same seed can give different floats with MSVC, GCC and Clang. It is also 2.5 KB of state, copied every time a `World` is copied.
  - Rejected: `rand()`. Global hidden state, so two worlds would share it, its quality and range depend on the platform, and it cannot be seeded per world.
- **The generator lives in the `World`.** A copied world copies its generator, so a test can copy a world and step both copies to the same result, as the determinism test already does. Restarting is a fresh world with a fresh generator, so nothing random carries over from the last attempt.
- **The session hands out the seeds.** The attempt count already lives in the `Session`; the seeds follow the same rule. Each attempt is a different fight, and the session's one seed still reproduces all of them.
  - Rejected: one seed for every attempt. The fight would replay the same way after every death, and its patterns could be learned as a script instead of as a reading of the boss.
  - Rejected: a fresh system seed for each attempt. Simpler, but a run of attempts could not be replayed.
- **The session draws seeds on its own stream.** With the same stream, the first world's numbers would start right where the session's drawing of seeds left off, and the two sequences would be tied together for no reason.
- **The seed shows in the Debug overlay.** When a fight goes wrong, the seed is what I need to play it again.
- **The generator is passed in, not reached for.** `StepBoss` takes a `Random&` instead of reading a global, so the boss tests choose their own seed, and a boss stepped without a world still draws its numbers the same way.
- **The boss always draws in the same order.** Every attack takes its numbers in the same sequence: the attack (unless it is already decided), the warning's length, the combo, then the next stalk. That order is part of what a seed means, so changing it changes every fight a seed gives, and it should only change on purpose.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Session seed | `std::random_device` in `Game::Game` (`src/app/Game.cpp`) | From the system, at every launch |
| Stream the session draws seeds from | `kSeedStream` (`src/core/Session.cpp`) | 7 |
| Default seed of a world or a session | `CreateWorld`, `StartSession` | 0 |

To replay a fight, pass the seed from the `F1` overlay to `CreateWorld` in a test, or to `StartSession` in `Game::Game`.

## Tests

- `tests/core/RandomTests.cpp`: the generator gives the reference PCG32 outputs for seed 42 on stream 54; the same seed gives the same numbers and another seed gives others; floats stay in [0, 1) and spread over the whole interval, and ranges stay in [min, max); a chance of 0 never happens, 1 always does, and a half happens about half the time.
- `tests/core/SessionTests.cpp`: every attempt gets its own seed, and the session's seed decides them all; the world's generator starts from its seed; and standing still dies over each of 20 seeds.
- `tests/core/AttackChoiceTests.cpp` and `tests/core/BossComboTests.cpp`: the same seed picks the same attacks and plays the same fight, and another seed plays another. See [The boss](BOSS.md#tests).

## Limitations

- **One generator for the whole world.** When the summoned hairballs start drawing numbers too, a change to them will change the boss's picks for the same seed. That is fine for a game; a replay system that must survive code changes would want one stream per user.
- **The app's seed comes from `std::random_device`**, which is not guaranteed to be random on every platform. On Windows with MSVC it is.
- **There is no way to type a seed in the game.** Replaying a fight means changing code. A seed field in the pause menu could come with the menus on day 11.
