#include <doctest/doctest.h>

#include "core/MouseAim.h"

namespace {

constexpr horde::Vec2 kView{1280.0f, 720.0f};

} // namespace

TEST_CASE("at sensitivity 1 the crosshair moves as far as the cursor would") {
    // On a 1920x1080 screen a game pixel is 1.5 screen pixels: 30 screen pixels are 20 game pixels.
    const horde::Vec2 moved = horde::MoveMouseAim({640.0f, 360.0f}, {30.0f, -15.0f}, 1.5f, 1.0f, kView);
    CHECK(moved.x == doctest::Approx(660.0f));
    CHECK(moved.y == doctest::Approx(350.0f));
}

TEST_CASE("the sensitivity scales how far the crosshair moves") {
    const horde::Vec2 fast = horde::MoveMouseAim({640.0f, 360.0f}, {10.0f, 0.0f}, 1.0f, 4.0f, kView);
    CHECK(fast.x == doctest::Approx(680.0f));
    const horde::Vec2 slow = horde::MoveMouseAim({640.0f, 360.0f}, {10.0f, 0.0f}, 1.0f, 0.5f, kView);
    CHECK(slow.x == doctest::Approx(645.0f));
}

TEST_CASE("the crosshair stops at the edges of the view") {
    const horde::Vec2 corner = horde::MoveMouseAim({1270.0f, 5.0f}, {100.0f, -100.0f}, 1.0f, 1.0f, kView);
    CHECK(corner.x == doctest::Approx(1280.0f));
    CHECK(corner.y == doctest::Approx(0.0f));
    // And comes straight back off it: nothing is stored past the edge.
    const horde::Vec2 back = horde::MoveMouseAim(corner, {-10.0f, 10.0f}, 1.0f, 1.0f, kView);
    CHECK(back.x == doctest::Approx(1270.0f));
    CHECK(back.y == doctest::Approx(10.0f));
}

TEST_CASE("a mouse that does not move leaves the crosshair where it is") {
    const horde::Vec2 still = horde::MoveMouseAim({300.0f, 200.0f}, {}, 2.0f, 3.0f, kView);
    CHECK(still.x == doctest::Approx(300.0f));
    CHECK(still.y == doctest::Approx(200.0f));
}
