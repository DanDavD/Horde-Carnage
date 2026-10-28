#include <doctest/doctest.h>

#include "core/TrailingBar.h"

using horde::TrailingBar;

namespace {

constexpr float kDt = 1.0f / 60.0f;
constexpr int kMax = 100;

// Steps the bar at a steady `health` for `steps` steps.
void Hold(TrailingBar& bar, int health, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepTrailingBar(bar, health, kMax, kDt);
    }
}

} // namespace

TEST_CASE("a fresh bar has no trail and no tally") {
    TrailingBar bar = horde::TrailingBarAt(kMax);
    CHECK_FALSE(horde::IsTrailing(bar, kMax));
    Hold(bar, kMax, 60);
    CHECK_FALSE(horde::IsTrailing(bar, kMax));
    CHECK(bar.tally == 0);
}

TEST_CASE("a hit leaves the trail where the bar was, waits, then drains down to the bar at its rate") {
    TrailingBar bar = horde::TrailingBarAt(kMax);
    Hold(bar, 70, 1);
    CHECK(bar.trail == doctest::Approx(100.0f));
    CHECK(bar.tally == 30);

    Hold(bar, 70, 35); // Just inside the 0.6 s hold.
    CHECK(bar.trail == doctest::Approx(100.0f));

    // Out of the hold, it drains 60 a second: 30 takes half a second.
    Hold(bar, 70, 10);
    CHECK(bar.trail < 100.0f);
    CHECK(bar.trail > 70.0f);
    Hold(bar, 70, 30);
    CHECK(bar.trail == doctest::Approx(70.0f));
    CHECK_FALSE(horde::IsTrailing(bar, 70));
    CHECK(bar.tally == 0);
}

TEST_CASE("hits that keep coming hold the trail and add up, and a hit after it caught up starts over") {
    TrailingBar bar = horde::TrailingBarAt(kMax);
    for (int health = 94; health >= 70; health -= 6) {
        Hold(bar, health, 6); // A bullet every 0.1 s, like the SMG.
    }
    CHECK(bar.trail == doctest::Approx(100.0f));
    CHECK(bar.tally == 30);

    Hold(bar, 70, 120);
    REQUIRE_FALSE(horde::IsTrailing(bar, 70));
    Hold(bar, 60, 1);
    CHECK(bar.tally == 10);
    CHECK(bar.trail == doctest::Approx(70.0f));
}

TEST_CASE("the drain never goes below the bar, even on a long frame") {
    TrailingBar bar = horde::TrailingBarAt(kMax);
    Hold(bar, 90, 1);
    Hold(bar, 90, 40);
    horde::StepTrailingBar(bar, 90, kMax, 5.0f);
    CHECK(bar.trail == doctest::Approx(90.0f));
}

TEST_CASE("a heal lifts the trail with the bar once it passes it, and not before") {
    TrailingBar bar = horde::TrailingBarAt(kMax);
    Hold(bar, 40, 1); // Trail at 100, bar at 40.
    Hold(bar, 80, 1); // A flask, still under the trail.
    CHECK(bar.trail == doctest::Approx(100.0f));
    CHECK(horde::IsTrailing(bar, 80));

    TrailingBar drained = horde::TrailingBarAt(40);
    Hold(drained, 80, 1);
    CHECK(drained.trail == doctest::Approx(80.0f));
    CHECK_FALSE(horde::IsTrailing(drained, 80));
    CHECK(drained.tally == 0);
}
