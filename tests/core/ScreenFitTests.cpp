#include <doctest/doctest.h>

#include "core/ScreenFit.h"

using horde::ScreenFit;
using horde::Vec2;

namespace {

constexpr Vec2 kGame{1280.0f, 720.0f};

} // namespace

TEST_CASE("a 16:9 screen fills with the game, scaled to its size") {
    const ScreenFit hd = horde::FitScreen(kGame, Vec2{1920.0f, 1080.0f});
    CHECK(hd.scale == doctest::Approx(1.5f));
    CHECK(hd.offset.x == doctest::Approx(0.0f));
    CHECK(hd.offset.y == doctest::Approx(0.0f));

    const ScreenFit uhd = horde::FitScreen(kGame, Vec2{3840.0f, 2160.0f});
    CHECK(uhd.scale == doctest::Approx(3.0f));

    const ScreenFit same = horde::FitScreen(kGame, kGame);
    CHECK(same.scale == doctest::Approx(1.0f));
}

TEST_CASE("a wider screen gets bars on the sides, a taller one above and below") {
    // 21:9 at 1440 lines: the height decides, and the rest of the width is split between two bars.
    const ScreenFit wide = horde::FitScreen(kGame, Vec2{3440.0f, 1440.0f});
    CHECK(wide.scale == doctest::Approx(2.0f));
    CHECK(wide.offset.x == doctest::Approx((3440.0f - 2560.0f) * 0.5f));
    CHECK(wide.offset.y == doctest::Approx(0.0f));

    // 16:10: the width decides.
    const ScreenFit tall = horde::FitScreen(kGame, Vec2{1920.0f, 1200.0f});
    CHECK(tall.scale == doctest::Approx(1.5f));
    CHECK(tall.offset.x == doctest::Approx(0.0f));
    CHECK(tall.offset.y == doctest::Approx(60.0f));
}

TEST_CASE("a screen point and a game point convert both ways") {
    const ScreenFit wide = horde::FitScreen(kGame, Vec2{3440.0f, 1440.0f});
    // The middle of the screen is the middle of the game.
    const Vec2 middle = horde::ScreenToGame(wide, Vec2{1720.0f, 720.0f});
    CHECK(middle.x == doctest::Approx(640.0f));
    CHECK(middle.y == doctest::Approx(360.0f));
    // The game's corner is where the bar ends.
    const Vec2 corner = horde::GameToScreen(wide, Vec2{0.0f, 0.0f});
    CHECK(corner.x == doctest::Approx(440.0f));
    // A point on the bar is outside the game.
    CHECK(horde::ScreenToGame(wide, Vec2{100.0f, 720.0f}).x < 0.0f);
    // And back again.
    const Vec2 back = horde::GameToScreen(wide, horde::ScreenToGame(wide, Vec2{2000.0f, 300.0f}));
    CHECK(back.x == doctest::Approx(2000.0f));
    CHECK(back.y == doctest::Approx(300.0f));
}

TEST_CASE("a screen with no area gives the identity fit") {
    const ScreenFit none = horde::FitScreen(kGame, Vec2{0.0f, 0.0f});
    CHECK(none.scale == doctest::Approx(1.0f));
    CHECK(none.offset.x == doctest::Approx(0.0f));
}
