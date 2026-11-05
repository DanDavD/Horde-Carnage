#include <doctest/doctest.h>

#include "core/AttackClock.h"

using horde::AttackClock;
using horde::AttackStage;

namespace {

// A quarter of a second is exact in binary, so these tests can compare times with ==.
constexpr float kStep = 0.25f;

void Tick(AttackClock& clock, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::TickClock(clock, kStep);
    }
}

} // namespace

TEST_CASE("an attack clock starts at the beginning of its warning") {
    const AttackClock clock;
    CHECK(clock.stage == AttackStage::Warning);
    CHECK(clock.seconds == 0.0f);
}

TEST_CASE("each tick adds its step to the stage, and a new stage starts from zero") {
    AttackClock clock;
    Tick(clock, 3);
    CHECK(clock.seconds == 0.75f);

    horde::EnterStage(clock, AttackStage::Active);
    CHECK(clock.stage == AttackStage::Active);
    CHECK(clock.seconds == 0.0f);

    Tick(clock, 1);
    CHECK(clock.seconds == kStep);
}

TEST_CASE("a stage of a whole number of steps ends on its last step, not one later") {
    AttackClock clock;
    Tick(clock, 2);
    CHECK_FALSE(horde::HasLasted(clock, 0.75f));
    Tick(clock, 1);
    CHECK(horde::HasLasted(clock, 0.75f));
}

TEST_CASE("the first beat falls at the start of the stage, then one every interval") {
    AttackClock clock;
    CHECK(horde::IsBeatDue(clock, 0, 0.5f));
    CHECK_FALSE(horde::IsBeatDue(clock, 1, 0.5f));

    Tick(clock, 1);
    CHECK_FALSE(horde::IsBeatDue(clock, 1, 0.5f));
    Tick(clock, 1);
    CHECK(horde::IsBeatDue(clock, 1, 0.5f));
    CHECK_FALSE(horde::IsBeatDue(clock, 2, 0.5f));
}

TEST_CASE("a series of beats is over once its last beat and the time after it have passed") {
    AttackClock clock;
    // Three beats half a second apart: the last one at 1 s, the series over at 1.25 s.
    Tick(clock, 4);
    CHECK_FALSE(horde::HasLastedPastBeats(clock, 3, 0.5f, 0.25f));
    Tick(clock, 1);
    CHECK(horde::HasLastedPastBeats(clock, 3, 0.5f, 0.25f));

    // A single beat falls at the start, so only the time after it counts.
    AttackClock single;
    CHECK_FALSE(horde::HasLastedPastBeats(single, 1, 0.5f, 0.25f));
    Tick(single, 1);
    CHECK(horde::HasLastedPastBeats(single, 1, 0.5f, 0.25f));
}
