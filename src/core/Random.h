#pragma once

#include <cstdint>

namespace horde {

/// A seeded random number generator: PCG32 (Melissa O'Neill, pcg-random.org), the XSH RR variant with
/// 64 bits of state and 32-bit outputs.
///
/// Everything random in the simulation draws from the `Random` inside the `World`, so a fight is decided
/// by its seed and the player's inputs alone: the same seed and the same inputs always give the same
/// fight. That keeps the tests deterministic and makes a bug found in one fight reproducible.
struct Random {
    std::uint64_t state = 0;
    std::uint64_t increment = 1; ///< Picks one of 2^63 independent streams. Always odd.
};

/// A generator seeded with `seed` on stream `stream`, set up exactly like the reference `pcg32_srandom_r`.
[[nodiscard]] Random SeedRandom(std::uint64_t seed, std::uint64_t stream = 54);

/// The next 32 random bits.
std::uint32_t NextU32(Random& random);

/// A float in [0, 1), from the top 24 bits, so every value is exactly representable.
float NextFloat(Random& random);

/// A float in [min, max).
float RandomRange(Random& random, float min, float max);

/// True with probability `probability`: never at 0 or below, always at 1 or above.
bool Chance(Random& random, float probability);

} // namespace horde
