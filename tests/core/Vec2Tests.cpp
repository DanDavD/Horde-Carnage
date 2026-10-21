#include <doctest/doctest.h>

#include "core/Vec2.h"

using horde::Vec2;

TEST_CASE("vectors add, subtract and scale component by component") {
    CHECK(Vec2{1.0f, 2.0f} + Vec2{3.0f, 4.0f} == Vec2{4.0f, 6.0f});
    CHECK(Vec2{3.0f, 4.0f} - Vec2{1.0f, 2.0f} == Vec2{2.0f, 2.0f});
    CHECK(Vec2{1.0f, -2.0f} * 3.0f == Vec2{3.0f, -6.0f});
    CHECK(0.5f * Vec2{4.0f, 8.0f} == Vec2{2.0f, 4.0f});
}

TEST_CASE("length of a 3-4-5 triangle") {
    CHECK(horde::LengthSquared(Vec2{3.0f, 4.0f}) == doctest::Approx(25.0f));
    CHECK(horde::Length(Vec2{3.0f, 4.0f}) == doctest::Approx(5.0f));
}

TEST_CASE("Dot is positive for the same direction, zero at 90 degrees and negative for opposites") {
    CHECK(horde::Dot(Vec2{2.0f, 0.0f}, Vec2{3.0f, 0.0f}) == doctest::Approx(6.0f));
    CHECK(horde::Dot(Vec2{1.0f, 0.0f}, Vec2{0.0f, 5.0f}) == doctest::Approx(0.0f));
    CHECK(horde::Dot(Vec2{1.0f, 1.0f}, Vec2{-1.0f, 0.0f}) == doctest::Approx(-1.0f));
}

TEST_CASE("Normalized keeps the direction at length 1, and a zero vector stays zero") {
    const Vec2 unit = horde::Normalized(Vec2{3.0f, 4.0f});
    CHECK(unit.x == doctest::Approx(0.6f));
    CHECK(unit.y == doctest::Approx(0.8f));
    CHECK(horde::Normalized(Vec2{}) == Vec2{});
}

TEST_CASE("ClampLength only shortens vectors that are too long") {
    CHECK(horde::ClampLength(Vec2{0.6f, 0.0f}, 1.0f) == Vec2{0.6f, 0.0f});

    const Vec2 clamped = horde::ClampLength(Vec2{3.0f, 4.0f}, 2.5f);
    CHECK(clamped.x == doctest::Approx(1.5f));
    CHECK(clamped.y == doctest::Approx(2.0f));
}

TEST_CASE("MoveTowards steps by at most maxDelta and never overshoots") {
    CHECK(horde::MoveTowards(Vec2{}, Vec2{10.0f, 0.0f}, 4.0f) == Vec2{4.0f, 0.0f});
    CHECK(horde::MoveTowards(Vec2{}, Vec2{10.0f, 0.0f}, 20.0f) == Vec2{10.0f, 0.0f});
    CHECK(horde::MoveTowards(Vec2{5.0f, 5.0f}, Vec2{5.0f, 5.0f}, 1.0f) == Vec2{5.0f, 5.0f});
}

TEST_CASE("Lerp goes from a to b") {
    const Vec2 a{0.0f, 10.0f};
    const Vec2 b{4.0f, 20.0f};
    CHECK(horde::Lerp(a, b, 0.0f) == a);
    CHECK(horde::Lerp(a, b, 1.0f) == b);
    CHECK(horde::Lerp(a, b, 0.5f) == Vec2{2.0f, 15.0f});
}
