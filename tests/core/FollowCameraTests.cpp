#include <cmath>

#include <doctest/doctest.h>

#include "core/FollowCamera.h"

using horde::Arena;
using horde::FollowCamera;
using horde::Vec2;

namespace {

// A 1000x600 arena seen through a 200x100 view, so the view center can go from (100, 50) to (900, 550).
constexpr Arena kArena{{0.0f, 0.0f}, {1000.0f, 600.0f}};
constexpr Vec2 kView{200.0f, 100.0f};
constexpr float kDt = 0.125f;
// No wall margin, so the expected centers below are easy to read. The margin has its own test.
constexpr horde::FollowCameraStats kStats{.wallMargin = 0.0f};

void Step(FollowCamera& camera, Vec2 target, Vec2 velocity, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepFollowCamera(camera, target, velocity, kArena, kDt);
    }
}

} // namespace

TEST_CASE("a view inside the arena is left where it is") {
    CHECK(horde::ClampViewToArena(Vec2{500.0f, 300.0f}, kView, kArena) == Vec2{500.0f, 300.0f});
}

TEST_CASE("a view near a wall stops where the edge of the screen meets the wall") {
    CHECK(horde::ClampViewToArena(Vec2{50.0f, 20.0f}, kView, kArena) == Vec2{100.0f, 50.0f});
    CHECK(horde::ClampViewToArena(Vec2{990.0f, 590.0f}, kView, kArena) == Vec2{900.0f, 550.0f});
}

TEST_CASE("on an axis where the arena is smaller than the view, the view is centred on the arena") {
    const Arena flat{{0.0f, 0.0f}, {1000.0f, 80.0f}};
    CHECK(horde::ClampViewToArena(Vec2{500.0f, 10.0f}, kView, flat) == Vec2{500.0f, 40.0f});
}

TEST_CASE("a new camera starts settled on its target, inside the arena") {
    const FollowCamera camera = horde::FollowCameraAt(Vec2{10.0f, 10.0f}, kView, kArena, kStats);
    CHECK(camera.center == Vec2{100.0f, 50.0f});
    CHECK(camera.previousCenter == Vec2{100.0f, 50.0f});
    CHECK(camera.viewSize == kView);
}

TEST_CASE("each step closes the fraction 1 - exp(-sharpness * dt) of the gap") {
    FollowCamera camera = horde::FollowCameraAt(Vec2{500.0f, 300.0f}, kView, kArena, kStats);
    Step(camera, Vec2{600.0f, 300.0f}, Vec2{}, 1);

    const float blend = 1.0f - std::exp(-camera.stats.sharpness * kDt);
    CHECK(camera.previousCenter == Vec2{500.0f, 300.0f});
    CHECK(camera.center.x == doctest::Approx(500.0f + 100.0f * blend));
    CHECK(camera.center.y == doctest::Approx(300.0f));
}

TEST_CASE("the camera settles on a target that stops") {
    FollowCamera camera = horde::FollowCameraAt(Vec2{500.0f, 300.0f}, kView, kArena, kStats);
    Step(camera, Vec2{600.0f, 350.0f}, Vec2{}, 100);
    CHECK(camera.center.x == doctest::Approx(600.0f));
    CHECK(camera.center.y == doctest::Approx(350.0f));
}

TEST_CASE("the camera looks ahead along the target's velocity") {
    FollowCamera camera = horde::FollowCameraAt(Vec2{500.0f, 300.0f}, kView, kArena, kStats);
    const Vec2 velocity{200.0f, 0.0f};
    Step(camera, Vec2{500.0f, 300.0f}, velocity, 100);
    CHECK(camera.center.x == doctest::Approx(500.0f + velocity.x * camera.stats.lookAhead));
    CHECK(camera.center.y == doctest::Approx(300.0f));
}

TEST_CASE("the look-ahead never shows anything past a wall, not even for one step") {
    FollowCamera camera = horde::FollowCameraAt(Vec2{880.0f, 300.0f}, kView, kArena, kStats);
    for (int i = 0; i < 100; ++i) {
        Step(camera, Vec2{880.0f, 300.0f}, Vec2{400.0f, 0.0f}, 1);
        REQUIRE(camera.center.x <= 900.0f);
    }
    CHECK(camera.center.x == doctest::Approx(900.0f));
}

TEST_CASE("the wall margin lets the view reach past the wall, so the wall is on screen") {
    const horde::FollowCameraStats stats{.wallMargin = 20.0f};
    FollowCamera camera = horde::FollowCameraAt(Vec2{0.0f, 0.0f}, kView, kArena, stats);
    CHECK(camera.center == Vec2{80.0f, 30.0f});

    Step(camera, Vec2{1000.0f, 600.0f}, Vec2{}, 100);
    CHECK(camera.center.x == doctest::Approx(920.0f));
    CHECK(camera.center.y == doctest::Approx(570.0f));
}
