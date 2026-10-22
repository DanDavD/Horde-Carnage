#include <doctest/doctest.h>

#include "core/Chase.h"

using horde::ChaseInput;
using horde::Vec2;

TEST_CASE("a chaser far from its target walks straight at it with full input") {
    const Vec2 input = ChaseInput(Vec2{0.0f, 0.0f}, Vec2{30.0f, 40.0f}, 10.0f);
    CHECK(input.x == doctest::Approx(0.6f));
    CHECK(input.y == doctest::Approx(0.8f));
}

TEST_CASE("just outside the stop distance a chaser still walks") {
    CHECK(ChaseInput(Vec2{0.0f, 0.0f}, Vec2{10.5f, 0.0f}, 10.0f) == Vec2{1.0f, 0.0f});
}

TEST_CASE("a chaser that touches its target stops pushing") {
    CHECK(ChaseInput(Vec2{0.0f, 0.0f}, Vec2{10.0f, 0.0f}, 10.0f) == Vec2{});
    CHECK(ChaseInput(Vec2{0.0f, 0.0f}, Vec2{4.0f, 3.0f}, 10.0f) == Vec2{});
}

TEST_CASE("a chaser exactly on its target gets no input instead of a broken direction") {
    CHECK(ChaseInput(Vec2{7.0f, 7.0f}, Vec2{7.0f, 7.0f}, 0.0f) == Vec2{});
}
