#include <doctest/doctest.h>

#include "core/attacks/Charge.h"

using horde::AttackStage;
using horde::BossAttack;
using horde::ChargeStats;
using horde::Vec2;

namespace {

// A sixty-fourth of a second is exact in binary, so every stage below lasts a whole number of steps.
constexpr float kDt = 1.0f / 64.0f;
constexpr horde::Arena kArena{{0.0f, 0.0f}, {2000.0f, 1000.0f}};

// A body and numbers that are not the Matriarch's: what a second boss would bring to the same attack.
// Half a second of warning (32 steps), 600 units/s over 300 units (32 steps of 9.375), and a quarter of a
// second to recover (16 steps).
constexpr ChargeStats kSmallCharge{.warningSeconds = 0.5f,
                                   .aimLockSeconds = 0.25f,
                                   .speed = 600.0f,
                                   .distance = 300.0f,
                                   .recoverySeconds = 0.25f,
                                   .stunSeconds = 1.0f};
constexpr float kRadius = 30.0f;

struct Body {
    horde::Motion motion;
    horde::MovementStats walk;
};

horde::Attacker AttackerOf(Body& body) {
    return horde::Attacker{.motion = body.motion, .walk = body.walk, .radius = kRadius};
}

// Steps the charge `steps` times and returns the last step's result.
horde::ChargeStep Step(BossAttack& charge, Body& body, Vec2 player, int steps) {
    horde::ChargeStep step;
    for (int i = 0; i < steps; ++i) {
        step = horde::StepCharge(charge, AttackerOf(body), kSmallCharge, player, kArena, kDt);
    }
    return step;
}

} // namespace

TEST_CASE("a charge with its own numbers aims, runs its own distance at its own speed and recovers") {
    Body body{.motion = horde::MotionAt(Vec2{500.0f, 500.0f})};
    BossAttack charge;
    const Vec2 player{900.0f, 500.0f};

    Step(charge, body, player, 31);
    CHECK(charge.clock.stage == AttackStage::Warning);
    Step(charge, body, player, 1);
    REQUIRE(charge.clock.stage == AttackStage::Active);
    CHECK(charge.direction == Vec2{1.0f, 0.0f});
    CHECK(charge.distanceLeft == kSmallCharge.distance);

    Step(charge, body, player, 31);
    CHECK(charge.clock.stage == AttackStage::Active);
    CHECK(body.motion.velocity.x == doctest::Approx(kSmallCharge.speed));
    const horde::ChargeStep last = Step(charge, body, player, 1);
    REQUIRE(charge.clock.stage == AttackStage::Recovery);
    CHECK(body.motion.position.x == doctest::Approx(800.0f));
    CHECK_FALSE(last.hitWall);
    CHECK_FALSE(charge.stunned);

    CHECK_FALSE(Step(charge, body, player, 15).finished);
    CHECK(Step(charge, body, player, 1).finished);
}

TEST_CASE("a charge that meets a wall stops against it, says so, and recovers for its stun time") {
    // 1800 units east, with 200 of room before the body of radius 30 touches the east wall at 1970.
    Body body{.motion = horde::MotionAt(Vec2{1800.0f, 500.0f})};
    BossAttack charge;
    const Vec2 player{1950.0f, 500.0f};

    Step(charge, body, player, 32);
    REQUIRE(charge.clock.stage == AttackStage::Active);
    bool hitWall = false;
    for (int i = 0; i < 40 && charge.clock.stage == AttackStage::Active; ++i) {
        hitWall = Step(charge, body, player, 1).hitWall;
    }
    REQUIRE(charge.clock.stage == AttackStage::Recovery);
    CHECK(hitWall);
    CHECK(charge.stunned);
    CHECK(body.motion.position.x == doctest::Approx(kArena.max.x - kRadius));

    // The stun lasts a second: 64 steps, against 16 after a run in the open.
    CHECK_FALSE(Step(charge, body, player, 63).finished);
    CHECK(Step(charge, body, player, 1).finished);
}

TEST_CASE("the aim locks for the last part of the warning, so a player who moves then escapes the line") {
    Body body{.motion = horde::MotionAt(Vec2{500.0f, 500.0f})};
    BossAttack charge;

    // East until the lock at 0.25 s, then the player steps south: the run still goes east.
    Step(charge, body, Vec2{900.0f, 500.0f}, 16);
    Step(charge, body, Vec2{500.0f, 900.0f}, 16);
    REQUIRE(charge.clock.stage == AttackStage::Active);
    CHECK(charge.direction == Vec2{1.0f, 0.0f});
}

TEST_CASE("a charge's warning is scaled by the attack, but never shorter than its aim lock") {
    BossAttack charge;
    charge.warningScale = 0.8f;
    CHECK(horde::ChargeWarningSeconds(kSmallCharge, charge) == doctest::Approx(0.4f));
    charge.warningScale = 0.1f;
    CHECK(horde::ChargeWarningSeconds(kSmallCharge, charge) == kSmallCharge.aimLockSeconds);
}
