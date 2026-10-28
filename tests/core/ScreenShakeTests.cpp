#include <algorithm>
#include <cmath>

#include <doctest/doctest.h>

#include "core/ScreenShake.h"

using horde::ScreenShake;
using horde::ShakeOffset;
using horde::Vec2;

namespace {

constexpr float kFrame = 1.0f / 144.0f;

} // namespace

TEST_CASE("without trauma the screen stays still") {
    const ScreenShake shake;
    const ShakeOffset now = horde::CurrentShake(shake);
    CHECK(now.offset == Vec2{});
    CHECK(now.turnDegrees == 0.0f);
}

TEST_CASE("trauma adds up to 1 and no further") {
    ScreenShake shake;
    horde::AddTrauma(shake, 0.4f);
    CHECK(shake.trauma == doctest::Approx(0.4f));
    horde::AddTrauma(shake, 0.4f);
    CHECK(shake.trauma == doctest::Approx(0.8f));
    horde::AddTrauma(shake, 0.4f);
    CHECK(shake.trauma == doctest::Approx(1.0f));
}

TEST_CASE("trauma wears off at its rate and the clock starts over once it is gone") {
    ScreenShake shake;
    horde::AddTrauma(shake, 1.0f);
    horde::StepScreenShake(shake, 0.2f);
    CHECK(shake.trauma == doctest::Approx(1.0f - shake.stats.decayPerSecond * 0.2f));
    CHECK(shake.seconds == doctest::Approx(0.2f));
    horde::StepScreenShake(shake, 1.0f);
    CHECK(shake.trauma == 0.0f);
    CHECK(shake.seconds == 0.0f);
    CHECK(horde::CurrentShake(shake).offset == Vec2{});
}

TEST_CASE("the shake grows with trauma squared, and never passes its limits") {
    ScreenShake full;
    horde::AddTrauma(full, 1.0f);
    ScreenShake half;
    horde::AddTrauma(half, 0.5f);
    // Freeze the trauma, so only the noise moves.
    full.stats.decayPerSecond = 0.0f;
    half.stats.decayPerSecond = 0.0f;
    float largestFull = 0.0f;
    float largestHalf = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        horde::StepScreenShake(full, kFrame);
        horde::StepScreenShake(half, kFrame);
        const ShakeOffset f = horde::CurrentShake(full);
        const ShakeOffset h = horde::CurrentShake(half);
        CHECK(std::abs(f.offset.x) <= full.stats.maxOffset);
        CHECK(std::abs(f.offset.y) <= full.stats.maxOffset);
        CHECK(std::abs(f.turnDegrees) <= full.stats.maxTurnDegrees);
        largestFull = std::max(largestFull, std::abs(f.offset.x));
        largestHalf = std::max(largestHalf, std::abs(h.offset.x));
        // Same clock, same noise: half the trauma is a quarter of the shake.
        CHECK(h.offset.x == doctest::Approx(f.offset.x * 0.25f).epsilon(0.001));
    }
    // Over a dozen seconds the noise reaches well toward the edge.
    CHECK(largestFull > full.stats.maxOffset * 0.6f);
    CHECK(largestHalf <= full.stats.maxOffset * 0.25f);
}

TEST_CASE("the shake sways from frame to frame instead of jumping") {
    ScreenShake shake;
    horde::AddTrauma(shake, 1.0f);
    shake.stats.decayPerSecond = 0.0f;
    horde::StepScreenShake(shake, kFrame);
    Vec2 last = horde::CurrentShake(shake).offset;
    // At 18 changes of direction a second, one frame at 144 Hz moves the view a fraction of the way.
    for (int i = 0; i < 1000; ++i) {
        horde::StepScreenShake(shake, kFrame);
        const Vec2 now = horde::CurrentShake(shake).offset;
        CHECK(horde::Length(now - last) < shake.stats.maxOffset * 0.5f);
        last = now;
    }
}

TEST_CASE("the same shake always moves the same way") {
    ScreenShake a;
    ScreenShake b;
    horde::AddTrauma(a, 0.7f);
    horde::AddTrauma(b, 0.7f);
    for (int i = 0; i < 50; ++i) {
        horde::StepScreenShake(a, kFrame);
        horde::StepScreenShake(b, kFrame);
    }
    CHECK(horde::CurrentShake(a).offset == horde::CurrentShake(b).offset);
}
