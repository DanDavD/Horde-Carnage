#include <doctest/doctest.h>

#include "core/Shockwave.h"

using horde::Shockwave;
using horde::ShockwaveStats;
using horde::Vec2;

namespace {

constexpr float kDt = 1.0f / 60.0f;

} // namespace

TEST_CASE("a shockwave grows at its speed and dies out past its reach") {
    const ShockwaveStats stats;
    Shockwave wave = horde::MakeShockwave(Vec2{}, 60.0f, stats);
    CHECK(horde::StepShockwave(wave, kDt));
    CHECK(wave.radius == doctest::Approx(60.0f + stats.speed * kDt));

    int steps = 1;
    while (horde::StepShockwave(wave, kDt)) {
        ++steps;
        REQUIRE(steps < 1000);
    }
    CHECK(wave.radius > stats.reach);
    // From 60 to 420 units at 380 units/s: a little under a second.
    CHECK(steps == doctest::Approx((stats.reach - 60.0f) / stats.speed / kDt).epsilon(0.03));
}

TEST_CASE("only the band of the ring touches: not inside it, not outside it") {
    const ShockwaveStats stats; // A band 20 units wide behind the front edge.
    const Shockwave wave = horde::MakeShockwave(Vec2{}, 200.0f, stats);
    constexpr float kBody = 20.0f;

    CHECK(horde::ShockwaveTouches(wave, Vec2{190.0f, 0.0f}, kBody)); // In the band.
    CHECK(horde::ShockwaveTouches(wave, Vec2{0.0f, 215.0f}, kBody)); // Just reached by the front.
    CHECK(horde::ShockwaveTouches(wave, Vec2{-165.0f, 0.0f}, kBody)); // Just behind the band.
    CHECK_FALSE(horde::ShockwaveTouches(wave, Vec2{225.0f, 0.0f}, kBody)); // Not reached yet.
    CHECK_FALSE(horde::ShockwaveTouches(wave, Vec2{150.0f, 0.0f}, kBody)); // Already passed.
    CHECK_FALSE(horde::ShockwaveTouches(wave, Vec2{}, kBody));             // In the middle.
}
