#include <doctest/doctest.h>

#include "core/Arena.h"

using horde::Arena;
using horde::Motion;
using horde::Vec2;

namespace {

// A 100x100 arena and a mover of radius 10, so its centre can go from 10 to 90 on each axis.
constexpr Arena kArena{{0.0f, 0.0f}, {100.0f, 100.0f}};
constexpr float kRadius = 10.0f;

} // namespace

TEST_CASE("the centre of an arena is halfway between its corners") {
    CHECK(horde::Center(Arena{{10.0f, 20.0f}, {30.0f, 60.0f}}) == Vec2{20.0f, 40.0f});
}

TEST_CASE("a mover inside the arena is left alone") {
    Motion motion{.position = {50.0f, 50.0f}, .velocity = {-30.0f, 20.0f}};
    horde::ConfineToArena(motion, kRadius, kArena);
    CHECK(motion.position == Vec2{50.0f, 50.0f});
    CHECK(motion.velocity == Vec2{-30.0f, 20.0f});
}

TEST_CASE("hitting a side wall stops only the push into it, so the mover slides along it") {
    Motion motion{.position = {5.0f, 50.0f}, .velocity = {-30.0f, 20.0f}};
    horde::ConfineToArena(motion, kRadius, kArena);
    CHECK(motion.position == Vec2{10.0f, 50.0f});
    CHECK(motion.velocity == Vec2{0.0f, 20.0f});
}

TEST_CASE("velocity pointing away from the wall is kept") {
    Motion motion{.position = {95.0f, 50.0f}, .velocity = {-30.0f, 0.0f}};
    horde::ConfineToArena(motion, kRadius, kArena);
    CHECK(motion.position == Vec2{90.0f, 50.0f});
    CHECK(motion.velocity == Vec2{-30.0f, 0.0f});
}

TEST_CASE("the top and bottom walls confine the vertical axis") {
    Motion top{.position = {50.0f, -20.0f}, .velocity = {0.0f, -50.0f}};
    horde::ConfineToArena(top, kRadius, kArena);
    CHECK(top.position == Vec2{50.0f, 10.0f});
    CHECK(top.velocity == Vec2{});

    Motion bottom{.position = {50.0f, 200.0f}, .velocity = {0.0f, 50.0f}};
    horde::ConfineToArena(bottom, kRadius, kArena);
    CHECK(bottom.position == Vec2{50.0f, 90.0f});
    CHECK(bottom.velocity == Vec2{});
}

TEST_CASE("a corner confines both axes at once") {
    Motion motion{.position = {120.0f, -5.0f}, .velocity = {40.0f, -40.0f}};
    horde::ConfineToArena(motion, kRadius, kArena);
    CHECK(motion.position == Vec2{90.0f, 10.0f});
    CHECK(motion.velocity == Vec2{});
}

TEST_CASE("an arena narrower than the mover keeps it in the middle") {
    Motion motion{.position = {0.0f, 50.0f}, .velocity = {10.0f, 0.0f}};
    horde::ConfineToArena(motion, kRadius, Arena{{0.0f, 0.0f}, {10.0f, 100.0f}});
    CHECK(motion.position == Vec2{5.0f, 50.0f});
    CHECK(motion.velocity == Vec2{});
}
