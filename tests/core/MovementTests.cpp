#include <doctest/doctest.h>

#include "core/Movement.h"

using horde::Motion;
using horde::MovementStats;
using horde::Vec2;

namespace {

// With these numbers every value below is exact in binary floating point: one step of 0.125 s
// adds 50 units/s of speed while accelerating and removes 100 units/s while stopping.
constexpr MovementStats kStats{.maxSpeed = 100.0f, .acceleration = 400.0f, .deceleration = 800.0f};
constexpr float kDt = 0.125f;

void Step(Motion& motion, Vec2 input, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepMovement(motion, input, kStats, kDt);
    }
}

} // namespace

TEST_CASE("with no input a mover at rest stays where it is") {
    Motion motion{.position = {5.0f, 5.0f}};
    Step(motion, Vec2{}, 10);
    CHECK(motion.position == Vec2{5.0f, 5.0f});
    CHECK(motion.velocity == Vec2{});
}

TEST_CASE("input accelerates to top speed and never past it") {
    Motion motion;
    Step(motion, Vec2{1.0f, 0.0f}, 1);
    CHECK(motion.velocity == Vec2{50.0f, 0.0f});

    Step(motion, Vec2{1.0f, 0.0f}, 1);
    CHECK(motion.velocity == Vec2{100.0f, 0.0f});

    Step(motion, Vec2{1.0f, 0.0f}, 20);
    CHECK(motion.velocity == Vec2{100.0f, 0.0f});
}

TEST_CASE("a keyboard diagonal is no faster than a straight line") {
    Motion motion;
    Step(motion, Vec2{1.0f, 1.0f}, 20);
    CHECK(horde::Length(motion.velocity) == doctest::Approx(kStats.maxSpeed));
    CHECK(motion.velocity.x == doctest::Approx(motion.velocity.y));
}

TEST_CASE("a half-pushed stick moves at half speed") {
    Motion motion;
    Step(motion, Vec2{0.0f, 0.5f}, 20);
    CHECK(motion.velocity == Vec2{0.0f, 50.0f});
}

TEST_CASE("releasing the input stops the mover without sliding back") {
    Motion motion{.velocity = {100.0f, 0.0f}};
    Step(motion, Vec2{}, 1);
    CHECK(motion.velocity == Vec2{});

    Step(motion, Vec2{}, 5);
    CHECK(motion.velocity == Vec2{});
}

TEST_CASE("reversing direction slows down through zero and speeds up the other way") {
    Motion motion{.velocity = {100.0f, 0.0f}};
    Step(motion, Vec2{-1.0f, 0.0f}, 2);
    CHECK(motion.velocity == Vec2{});

    Step(motion, Vec2{-1.0f, 0.0f}, 2);
    CHECK(motion.velocity == Vec2{-100.0f, 0.0f});
}

TEST_CASE("a step moves by the new velocity and remembers the old position") {
    Motion motion{.position = {10.0f, 10.0f}, .velocity = {100.0f, 0.0f}};
    Step(motion, Vec2{1.0f, 0.0f}, 1);
    CHECK(motion.previousPosition == Vec2{10.0f, 10.0f});
    CHECK(motion.position == Vec2{22.5f, 10.0f});
}
