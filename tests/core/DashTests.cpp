#include <doctest/doctest.h>

#include "core/Dash.h"

using horde::Dash;
using horde::Vec2;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// Runs a dash until its burst is over and returns how far it moved in total.
Vec2 RunBurst(Dash& dash) {
    Vec2 moved;
    for (int i = 0; i < 60 && horde::IsDashing(dash); ++i) {
        moved += horde::StepDash(dash, kDt);
        horde::TickDashCooldown(dash, kDt);
    }
    return moved;
}

} // namespace

TEST_CASE("a dash starts toward the given direction, at length 1") {
    Dash dash;
    CHECK(horde::TryStartDash(dash, Vec2{5.0f, 5.0f}, Vec2{3.0f, 4.0f}));
    CHECK(horde::IsDashing(dash));
    CHECK(horde::IsDashInvulnerable(dash));
    CHECK(dash.direction.x == doctest::Approx(0.6f));
    CHECK(dash.direction.y == doctest::Approx(0.8f));
    CHECK(dash.origin == Vec2{5.0f, 5.0f});
}

TEST_CASE("a dash with no direction does not start") {
    Dash dash;
    CHECK_FALSE(horde::TryStartDash(dash, Vec2{}, Vec2{}));
    CHECK_FALSE(horde::IsDashing(dash));
    CHECK(dash.cooldownSeconds == 0.0f);
}

TEST_CASE("a dash covers exactly its distance, even when the burst is not a whole number of steps") {
    // 0.12 s at 60 Hz is 7.2 steps: the eighth step moves only for the time that was left.
    Dash dash;
    REQUIRE(horde::TryStartDash(dash, Vec2{}, Vec2{1.0f, 0.0f}));
    const Vec2 moved = RunBurst(dash);
    CHECK(moved.x == doctest::Approx(dash.stats.distance));
    CHECK(moved.y == doctest::Approx(0.0f));
}

TEST_CASE("the invulnerability outlasts the burst a little, then ends") {
    Dash dash;
    REQUIRE(horde::TryStartDash(dash, Vec2{}, Vec2{1.0f, 0.0f}));
    RunBurst(dash);
    CHECK_FALSE(horde::IsDashing(dash));
    CHECK(horde::IsDashInvulnerable(dash));

    for (int i = 0; i < 3; ++i) {
        CHECK(horde::StepDash(dash, kDt) == Vec2{});
    }
    CHECK_FALSE(horde::IsDashInvulnerable(dash));
}

TEST_CASE("the next dash waits for the cooldown") {
    Dash dash;
    REQUIRE(horde::TryStartDash(dash, Vec2{}, Vec2{1.0f, 0.0f}));
    RunBurst(dash);
    CHECK_FALSE(horde::TryStartDash(dash, Vec2{}, Vec2{1.0f, 0.0f}));

    // The burst (0.12 s) and the cooldown (0.5 s) both count from the start: 0.62 s in total.
    horde::TickDashCooldown(dash, 0.5f);
    CHECK(horde::TryStartDash(dash, Vec2{}, Vec2{1.0f, 0.0f}));
}

TEST_CASE("not dashing moves nothing") {
    Dash dash;
    CHECK(horde::StepDash(dash, kDt) == Vec2{});
}
