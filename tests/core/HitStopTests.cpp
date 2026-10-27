#include <algorithm>

#include <doctest/doctest.h>

#include "core/HitStop.h"

using horde::HitStop;

namespace {

constexpr float kFrame = 1.0f / 144.0f;

// How many seconds of the fight pass while the slow-down plays out, frame by frame.
float FightSecondsDuring(HitStop& stop) {
    float fight = 0.0f;
    for (int i = 0; i < 10000 && stop.secondsLeft > 0.0f; ++i) {
        fight += kFrame * horde::TimeScale(stop);
        horde::StepHitStop(stop, kFrame);
    }
    return fight;
}

} // namespace

TEST_CASE("without a hit the fight runs at full speed") {
    HitStop stop;
    CHECK(horde::TimeScale(stop) == 1.0f);
    horde::StepHitStop(stop, kFrame);
    CHECK(horde::TimeScale(stop) == 1.0f);
}

TEST_CASE("a hit drops the fight to a crawl, never to a stop, and eases it back to full speed") {
    HitStop stop;
    horde::StartHitStop(stop, 0.3f);
    CHECK(horde::TimeScale(stop) == doctest::Approx(stop.stats.slowestScale));
    CHECK(horde::TimeScale(stop) > 0.0f);

    float last = horde::TimeScale(stop);
    float largestJump = 0.0f;
    while (stop.secondsLeft > 0.0f) {
        horde::StepHitStop(stop, kFrame);
        const float now = horde::TimeScale(stop);
        CHECK(now >= last);
        largestJump = std::max(largestJump, now - last);
        last = now;
    }
    CHECK(last == 1.0f);
    // A 0.3 s slow-down at 144 frames a second is 43 frames; eased, no frame speeds up by more than about a
    // tenth of full speed, so the return never snaps.
    CHECK(largestJump < 0.08f);
}

TEST_CASE("the fight loses about half the slow-down's length") {
    HitStop stop;
    horde::StartHitStop(stop, 0.4f);
    // Smoothstep averages one half, and the slowest speed adds a little on top.
    const float expected = 0.4f * (0.5f + 0.5f * stop.stats.slowestScale);
    CHECK(FightSecondsDuring(stop) == doctest::Approx(expected).epsilon(0.02));
}

TEST_CASE("a stronger hit takes over, and a weaker one during a stronger adds nothing") {
    HitStop stop;
    horde::StartHitStop(stop, 0.3f);
    horde::StepHitStop(stop, 0.1f);
    const float before = horde::TimeScale(stop);

    horde::StartHitStop(stop, 0.1f); // Less than the 0.2 s left.
    CHECK(stop.secondsLeft == doctest::Approx(0.2f));
    CHECK(horde::TimeScale(stop) == doctest::Approx(before));

    horde::StartHitStop(stop, 0.6f); // A death: back to the slowest, for its whole length.
    CHECK(stop.secondsLeft == doctest::Approx(0.6f));
    CHECK(stop.seconds == doctest::Approx(0.6f));
    CHECK(horde::TimeScale(stop) == doctest::Approx(stop.stats.slowestScale));
}
