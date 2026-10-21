#include <doctest/doctest.h>

#include "core/AnalogInput.h"

using horde::ApplyRadialDeadzone;
using horde::Vec2;

namespace {

constexpr float kDeadzone = 0.25f;

} // namespace

TEST_CASE("stick drift inside the deadzone reads as no input") {
    CHECK(ApplyRadialDeadzone(Vec2{0.1f, -0.15f}, kDeadzone) == Vec2{});
    CHECK(ApplyRadialDeadzone(Vec2{0.25f, 0.0f}, kDeadzone) == Vec2{});
}

TEST_CASE("values past the deadzone are rescaled to start from zero") {
    // Halfway between the deadzone edge (0.25) and full tilt (1) gives half speed.
    const Vec2 result = ApplyRadialDeadzone(Vec2{0.625f, 0.0f}, kDeadzone);
    CHECK(result.x == doctest::Approx(0.5f));
    CHECK(result.y == doctest::Approx(0.0f));
}

TEST_CASE("full tilt stays at length 1") {
    const Vec2 result = ApplyRadialDeadzone(Vec2{0.0f, -1.0f}, kDeadzone);
    CHECK(result.x == doctest::Approx(0.0f));
    CHECK(result.y == doctest::Approx(-1.0f));
}

TEST_CASE("the direction of a diagonal is kept") {
    const Vec2 result = ApplyRadialDeadzone(Vec2{0.48f, 0.64f}, kDeadzone);
    CHECK(result.x / result.y == doctest::Approx(0.75f));
    CHECK(horde::Length(result) == doctest::Approx((0.8f - kDeadzone) / (1.0f - kDeadzone)));
}

TEST_CASE("a gamepad corner longer than 1 is capped at length 1") {
    const Vec2 result = ApplyRadialDeadzone(Vec2{1.0f, 1.0f}, kDeadzone);
    CHECK(horde::Length(result) == doctest::Approx(1.0f));
    CHECK(result.x == doctest::Approx(result.y));
}
