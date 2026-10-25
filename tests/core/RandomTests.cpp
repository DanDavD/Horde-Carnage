#include <algorithm>
#include <cstdint>

#include <doctest/doctest.h>

#include "core/Random.h"

using horde::Random;

TEST_CASE("the generator gives the reference PCG32 outputs for seed 42 on stream 54") {
    // The first outputs of the reference implementation's demo, pcg32_srandom_r(&rng, 42, 54).
    Random random = horde::SeedRandom(42, 54);
    CHECK(horde::NextU32(random) == 0xa15c02b7u);
    CHECK(horde::NextU32(random) == 0x7b47f409u);
    CHECK(horde::NextU32(random) == 0xba1d3330u);
    CHECK(horde::NextU32(random) == 0x83d2f293u);
    CHECK(horde::NextU32(random) == 0xbfa4784bu);
    CHECK(horde::NextU32(random) == 0xcbed606eu);
}

TEST_CASE("the same seed gives the same numbers, and another seed gives others") {
    Random a = horde::SeedRandom(1234);
    Random b = horde::SeedRandom(1234);
    Random c = horde::SeedRandom(1235);
    int matches = 0;
    int differences = 0;
    for (int i = 0; i < 100; ++i) {
        const std::uint32_t fromA = horde::NextU32(a);
        matches += fromA == horde::NextU32(b) ? 1 : 0;
        differences += fromA != horde::NextU32(c) ? 1 : 0;
    }
    CHECK(matches == 100);
    CHECK(differences > 95);
}

TEST_CASE("floats stay in [0, 1) and ranges stay in [min, max)") {
    Random random = horde::SeedRandom(7);
    float lowest = 1.0f;
    float highest = 0.0f;
    int outside = 0;
    for (int i = 0; i < 10000; ++i) {
        const float value = horde::NextFloat(random);
        lowest = std::min(lowest, value);
        highest = std::max(highest, value);
        const float ranged = horde::RandomRange(random, -2.0f, 3.0f);
        outside += ranged < -2.0f || ranged >= 3.0f ? 1 : 0;
    }
    CHECK(lowest >= 0.0f);
    CHECK(highest < 1.0f);
    CHECK(outside == 0);
    // Spread over the whole interval, not bunched at one end.
    CHECK(lowest < 0.01f);
    CHECK(highest > 0.99f);
}

TEST_CASE("a chance of 0 never happens, 1 always does, and a half happens about half the time") {
    Random random = horde::SeedRandom(99);
    int never = 0;
    int always = 0;
    int halves = 0;
    for (int i = 0; i < 10000; ++i) {
        never += horde::Chance(random, 0.0f) ? 1 : 0;
        always += horde::Chance(random, 1.0f) ? 1 : 0;
        halves += horde::Chance(random, 0.5f) ? 1 : 0;
    }
    CHECK(never == 0);
    CHECK(always == 10000);
    CHECK(halves > 4800);
    CHECK(halves < 5200);
}
