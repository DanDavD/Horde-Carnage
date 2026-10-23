#include <optional>

#include <doctest/doctest.h>

#include "core/Collision.h"

using horde::Motion;
using horde::Vec2;

namespace {

// Two bodies of radius 10, so they touch when their centres are 20 apart.
constexpr float kRadius = 10.0f;

bool Separate(Motion& a, Motion& b, float shareA) {
    return horde::SeparateCircles(a, kRadius, b, kRadius, shareA);
}

} // namespace

TEST_CASE("the heavier body takes the smaller share of the push") {
    CHECK(horde::PushShare(1.0f, 1.0f) == doctest::Approx(0.5f));
    CHECK(horde::PushShare(4.0f, 1.0f) == doctest::Approx(0.2f));
    CHECK(horde::PushShare(1.0f, 3.0f) == doctest::Approx(0.75f));
}

TEST_CASE("bodies that are apart are left alone") {
    Motion a{.position = {0.0f, 0.0f}, .velocity = {5.0f, 0.0f}};
    Motion b{.position = {30.0f, 0.0f}, .velocity = {-5.0f, 0.0f}};
    CHECK_FALSE(Separate(a, b, 0.5f));
    CHECK(a.position == Vec2{0.0f, 0.0f});
    CHECK(b.position == Vec2{30.0f, 0.0f});
    CHECK(a.velocity == Vec2{5.0f, 0.0f});
}

TEST_CASE("bodies that just touch do not count as overlapping") {
    Motion a{.position = {0.0f, 0.0f}};
    Motion b{.position = {20.0f, 0.0f}};
    CHECK_FALSE(Separate(a, b, 0.5f));
}

TEST_CASE("overlapping bodies of equal weight move apart by half the overlap each") {
    Motion a{.position = {0.0f, 0.0f}};
    Motion b{.position = {16.0f, 0.0f}};
    CHECK(Separate(a, b, 0.5f));
    CHECK(a.position == Vec2{-2.0f, 0.0f});
    CHECK(b.position == Vec2{18.0f, 0.0f});
}

TEST_CASE("the heavier body moves less") {
    Motion a{.position = {0.0f, 0.0f}};
    Motion b{.position = {16.0f, 0.0f}};
    CHECK(Separate(a, b, 0.25f));
    CHECK(a.position == Vec2{-1.0f, 0.0f});
    CHECK(b.position == Vec2{19.0f, 0.0f});
}

TEST_CASE("two bodies on the same point are pushed apart along x, the same way every time") {
    Motion a{.position = {5.0f, 5.0f}};
    Motion b{.position = {5.0f, 5.0f}};
    CHECK(Separate(a, b, 0.5f));
    CHECK(a.position == Vec2{-5.0f, 5.0f});
    CHECK(b.position == Vec2{15.0f, 5.0f});
}

TEST_CASE("a heavy body running into a still one shoves it, and both end at the same speed") {
    // Weights 4 and 1: a perfectly inelastic collision ends at (4 * 300 + 1 * 0) / 5 = 240 for both.
    Motion player{.position = {0.0f, 0.0f}, .velocity = {300.0f, 0.0f}};
    Motion hairball{.position = {15.0f, 0.0f}};
    CHECK(Separate(player, hairball, horde::PushShare(4.0f, 1.0f)));
    CHECK(player.velocity.x == doctest::Approx(240.0f));
    CHECK(hairball.velocity.x == doctest::Approx(240.0f));
}

TEST_CASE("bodies that already move apart keep their speed") {
    Motion a{.position = {0.0f, 0.0f}, .velocity = {-50.0f, 0.0f}};
    Motion b{.position = {16.0f, 0.0f}, .velocity = {50.0f, 0.0f}};
    CHECK(Separate(a, b, 0.5f));
    CHECK(a.velocity == Vec2{-50.0f, 0.0f});
    CHECK(b.velocity == Vec2{50.0f, 0.0f});
}

TEST_CASE("speed across the contact is kept, so bodies slide past each other") {
    Motion a{.position = {0.0f, 0.0f}, .velocity = {0.0f, 100.0f}};
    Motion b{.position = {16.0f, 0.0f}};
    CHECK(Separate(a, b, 0.5f));
    CHECK(a.velocity == Vec2{0.0f, 100.0f});
    CHECK(b.velocity == Vec2{});
}

TEST_CASE("bodies count as touching up to the contact slack, and not past it") {
    // Radii 10 and 10: they touch at 20, and the slack allows up to 21.
    CHECK(horde::AreTouching(Vec2{0.0f, 0.0f}, kRadius, Vec2{20.0f, 0.0f}, kRadius));
    CHECK(horde::AreTouching(Vec2{0.0f, 0.0f}, kRadius, Vec2{20.0f + horde::kContactSlack, 0.0f}, kRadius));
    CHECK_FALSE(horde::AreTouching(Vec2{0.0f, 0.0f}, kRadius, Vec2{22.0f, 0.0f}, kRadius));
}

TEST_CASE("a moving circle sweeping into a still one touches it partway along the move") {
    // Radius 2 moving from x = 0 to x = 100, toward a radius 8 circle at x = 50: they touch at x = 40.
    const std::optional<float> fraction = horde::SweepCircle(Vec2{0.0f, 0.0f}, Vec2{100.0f, 0.0f}, 2.0f,
                                                             Vec2{50.0f, 0.0f}, 8.0f);
    REQUIRE(fraction.has_value());
    CHECK(*fraction == doctest::Approx(0.4f));
}

TEST_CASE("a sweep that passes beside a circle, or stops short of it, misses") {
    CHECK_FALSE(horde::SweepCircle(Vec2{0.0f, 20.0f}, Vec2{100.0f, 20.0f}, 2.0f, Vec2{50.0f, 0.0f}, 8.0f));
    CHECK_FALSE(horde::SweepCircle(Vec2{0.0f, 0.0f}, Vec2{30.0f, 0.0f}, 2.0f, Vec2{50.0f, 0.0f}, 8.0f));
}

TEST_CASE("a sweep that starts already touching hits at once") {
    const std::optional<float> fraction = horde::SweepCircle(Vec2{45.0f, 0.0f}, Vec2{100.0f, 0.0f}, 2.0f,
                                                             Vec2{50.0f, 0.0f}, 8.0f);
    REQUIRE(fraction.has_value());
    CHECK(*fraction == 0.0f);
}

TEST_CASE("a fast circle cannot skip over a thin one between two steps") {
    // Both ends of the move are clear of the circle, but the path goes straight through it.
    CHECK(horde::SweepCircle(Vec2{0.0f, 0.0f}, Vec2{100.0f, 0.0f}, 1.0f, Vec2{50.0f, 0.0f}, 2.0f).has_value());
}

TEST_CASE("a circle that does not move only hits what it already touches") {
    CHECK_FALSE(horde::SweepCircle(Vec2{0.0f, 0.0f}, Vec2{0.0f, 0.0f}, 2.0f, Vec2{50.0f, 0.0f}, 8.0f));
}
