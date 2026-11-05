#include <doctest/doctest.h>

#include "core/attacks/Leap.h"

using horde::AttackStage;
using horde::BossAttack;
using horde::LeapStats;
using horde::Vec2;

namespace {

// A sixty-fourth of a second is exact in binary, so every stage below lasts a whole number of steps.
constexpr float kDt = 1.0f / 64.0f;
constexpr horde::Arena kArena{{0.0f, 0.0f}, {2000.0f, 1000.0f}};
constexpr float kRadius = 30.0f;

// A body and numbers that are not the Matriarch's. A quarter of a second crouched (16 steps), half a second
// in the air (32 steps) with the shadow locked for the last quarter, an eighth on landing (8 steps) and a
// quarter to recover (16 steps).
constexpr LeapStats kQuickLeap{.crouchSeconds = 0.25f,
                               .airSeconds = 0.5f,
                               .lockSeconds = 0.25f,
                               .shadowSpeed = 400.0f,
                               .landingSeconds = 0.125f,
                               .recoverySeconds = 0.25f};

struct Body {
    horde::Motion motion;
    horde::MovementStats walk;
};

horde::Attacker AttackerOf(Body& body) {
    return horde::Attacker{.motion = body.motion, .walk = body.walk, .radius = kRadius};
}

horde::LeapStep Step(BossAttack& leap, Body& body, Vec2 player, int steps) {
    horde::LeapStep step;
    for (int i = 0; i < steps; ++i) {
        step = horde::StepLeap(leap, AttackerOf(body), kQuickLeap, player, kArena, kDt);
    }
    return step;
}

} // namespace

TEST_CASE("a leap with its own numbers crouches, flies to its shadow and lands after its own air time") {
    Body body{.motion = horde::MotionAt(Vec2{500.0f, 500.0f})};
    BossAttack leap;
    const Vec2 player{700.0f, 500.0f};
    horde::AimLeap(leap, body.motion.position, player, kRadius, kArena);
    CHECK(leap.target == player);

    Step(leap, body, player, 16);
    CHECK_FALSE(horde::IsLeapAirborne(kQuickLeap, leap));
    CHECK(body.motion.position == Vec2{500.0f, 500.0f});

    Step(leap, body, player, 1);
    CHECK(horde::IsLeapAirborne(kQuickLeap, leap));
    CHECK(body.motion.position.x > 500.0f);

    CHECK_FALSE(Step(leap, body, player, 30).landed);
    const horde::LeapStep landing = Step(leap, body, player, 1);
    CHECK(landing.landed);
    CHECK(leap.clock.stage == AttackStage::Active);
    CHECK(body.motion.position == player);
    CHECK_FALSE(horde::IsLeapAirborne(kQuickLeap, leap));

    Step(leap, body, player, 8);
    CHECK(leap.clock.stage == AttackStage::Recovery);
    CHECK_FALSE(Step(leap, body, player, 15).finished);
    CHECK(Step(leap, body, player, 1).finished);
}

TEST_CASE("a leap's shadow chases the player at its own speed until it locks") {
    Body body{.motion = horde::MotionAt(Vec2{500.0f, 500.0f})};
    BossAttack leap;
    horde::AimLeap(leap, body.motion.position, Vec2{700.0f, 500.0f}, kRadius, kArena);

    // In the air the shadow follows at 400 units/s, 6.25 a step, for the 16 steps before the lock.
    const Vec2 farAway{1500.0f, 500.0f};
    Step(leap, body, farAway, 16 + 16);
    CHECK(leap.target.x == doctest::Approx(700.0f + 16.0f * 6.25f));
    const float locked = leap.target.x;
    Step(leap, body, farAway, 8);
    CHECK(leap.target.x == locked);
}

TEST_CASE("a leap aimed at a player against a wall lands with the whole body inside it") {
    BossAttack leap;
    horde::AimLeap(leap, Vec2{500.0f, 500.0f}, Vec2{1995.0f, 5.0f}, kRadius, kArena);
    CHECK(leap.target == Vec2{kArena.max.x - kRadius, kArena.min.y + kRadius});
}
