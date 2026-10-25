#include "core/Random.h"

namespace horde {

namespace {

constexpr std::uint64_t kMultiplier = 6364136223846793005ULL;

} // namespace

Random SeedRandom(std::uint64_t seed, std::uint64_t stream) {
    Random random{.state = 0, .increment = (stream << 1u) | 1u};
    NextU32(random);
    random.state += seed;
    NextU32(random);
    return random;
}

std::uint32_t NextU32(Random& random) {
    const std::uint64_t old = random.state;
    random.state = old * kMultiplier + random.increment;
    // The output permutation: an xorshift of the high bits, then a rotation chosen by the top five bits.
    const auto xorShifted = static_cast<std::uint32_t>(((old >> 18u) ^ old) >> 27u);
    const auto rotation = static_cast<std::uint32_t>(old >> 59u);
    return (xorShifted >> rotation) | (xorShifted << ((32u - rotation) & 31u));
}

float NextFloat(Random& random) {
    return static_cast<float>(NextU32(random) >> 8u) * (1.0f / 16777216.0f);
}

float RandomRange(Random& random, float min, float max) {
    return min + (max - min) * NextFloat(random);
}

bool Chance(Random& random, float probability) {
    return NextFloat(random) < probability;
}

} // namespace horde
