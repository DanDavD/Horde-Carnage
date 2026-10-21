#include <doctest/doctest.h>

#include "core/Facing.h"
#include "core/Player.h"

using horde::Vec2;

TEST_CASE("a new player faces down, toward the camera") {
    CHECK(horde::Player{}.facing == Vec2{0.0f, 1.0f});
}

TEST_CASE("input turns the facing to its direction, at length 1") {
    Vec2 facing{0.0f, 1.0f};
    horde::UpdateFacing(facing, Vec2{3.0f, 4.0f});
    CHECK(facing.x == doctest::Approx(0.6f));
    CHECK(facing.y == doctest::Approx(0.8f));
}

TEST_CASE("with no input the last facing is kept") {
    Vec2 facing{-1.0f, 0.0f};
    horde::UpdateFacing(facing, Vec2{});
    CHECK(facing == Vec2{-1.0f, 0.0f});
}

TEST_CASE("a keyboard diagonal faces diagonally") {
    Vec2 facing{0.0f, 1.0f};
    horde::UpdateFacing(facing, Vec2{1.0f, -1.0f});
    CHECK(facing.x == doctest::Approx(0.70710678f));
    CHECK(facing.y == doctest::Approx(-0.70710678f));
}

TEST_CASE("a small stick push turns the player as fully as full tilt") {
    Vec2 facing{0.0f, 1.0f};
    horde::UpdateFacing(facing, Vec2{0.05f, 0.0f});
    CHECK(facing.x == doctest::Approx(1.0f));
    CHECK(facing.y == doctest::Approx(0.0f));
}
