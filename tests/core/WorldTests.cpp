#include <cstddef>
#include <vector>

#include <doctest/doctest.h>

#include "core/World.h"

using horde::Arena;
using horde::Enemy;
using horde::Vec2;
using horde::World;

namespace {

// The same arena and view as the game, so these tests describe what the player actually sees.
constexpr Arena kArena{{0.0f, 0.0f}, {2400.0f, 1600.0f}};
constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

void Step(World& world, Vec2 input, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepWorld(world, input, kDt);
    }
}

float DistanceToPlayer(const World& world, const Enemy& enemy) {
    return horde::Length(enemy.motion.position - world.player.motion.position);
}

bool InsideArena(Vec2 position, float radius) {
    return position.x >= kArena.min.x + radius && position.x <= kArena.max.x - radius &&
           position.y >= kArena.min.y + radius && position.y <= kArena.max.y - radius;
}

} // namespace

TEST_CASE("a new world puts the player in the middle and the hairballs on a ring around it") {
    const World world = horde::CreateWorld(kArena, kView);
    CHECK(world.player.motion.position == Vec2{1200.0f, 800.0f});
    CHECK(world.player.motion.previousPosition == world.player.motion.position);

    REQUIRE(world.enemies.size() == static_cast<std::size_t>(horde::kStartingHairballs));
    for (const Enemy& enemy : world.enemies) {
        CHECK(DistanceToPlayer(world, enemy) == doctest::Approx(horde::kHairballSpawnDistance));
        CHECK(enemy.motion.previousPosition == enemy.motion.position);
        CHECK(enemy.motion.velocity == Vec2{});
        CHECK(InsideArena(enemy.motion.position, enemy.radius));
    }
}

TEST_CASE("the camera of a new world is already on the player") {
    const World world = horde::CreateWorld(kArena, kView);
    CHECK(world.camera.center == world.player.motion.position);
    CHECK(world.camera.previousCenter == world.camera.center);
}

TEST_CASE("a step moves and turns the player by the movement rules") {
    World world = horde::CreateWorld(kArena, kView);
    horde::Motion expected = world.player.motion;
    horde::StepMovement(expected, Vec2{1.0f, 0.0f}, world.player.stats, kDt);

    Step(world, Vec2{1.0f, 0.0f}, 1);
    CHECK(world.player.motion.position == expected.position);
    CHECK(world.player.motion.velocity == expected.velocity);
    CHECK(world.player.facing == Vec2{1.0f, 0.0f});
}

TEST_CASE("a step moves the camera after the player") {
    World world = horde::CreateWorld(kArena, kView);
    const Vec2 before = world.camera.center;
    Step(world, Vec2{1.0f, 0.0f}, 1);
    CHECK(world.camera.previousCenter == before);
    CHECK(world.camera.center.x > before.x);
}

TEST_CASE("hairballs walk toward the player") {
    World world = horde::CreateWorld(kArena, kView);
    std::vector<float> before;
    for (const Enemy& enemy : world.enemies) {
        before.push_back(DistanceToPlayer(world, enemy));
    }

    Step(world, Vec2{}, 60);
    for (std::size_t i = 0; i < world.enemies.size(); ++i) {
        CHECK(DistanceToPlayer(world, world.enemies[i]) < before[i]);
    }
}

TEST_CASE("hairballs stop when they touch the player instead of pushing into its centre") {
    World world = horde::CreateWorld(kArena, kView);
    Step(world, Vec2{}, 600);

    for (const Enemy& enemy : world.enemies) {
        const float contact = enemy.radius + world.player.radius;
        // The last full step and the braking after it can carry a hairball a few units past contact.
        CHECK(DistanceToPlayer(world, enemy) <= contact);
        CHECK(DistanceToPlayer(world, enemy) > contact - 10.0f);
        CHECK(enemy.motion.velocity == Vec2{});
    }
}

TEST_CASE("nobody leaves the arena when the player runs into a corner") {
    World world = horde::CreateWorld(kArena, kView);
    Step(world, Vec2{1.0f, 1.0f}, 600);

    CHECK(InsideArena(world.player.motion.position, world.player.radius));
    for (const Enemy& enemy : world.enemies) {
        CHECK(InsideArena(enemy.motion.position, enemy.radius));
    }
}

TEST_CASE("the same inputs always give the same world") {
    World a = horde::CreateWorld(kArena, kView);
    World b = horde::CreateWorld(kArena, kView);
    const Vec2 inputs[] = {{1.0f, 0.0f}, {0.0f, -1.0f}, {-0.5f, 0.5f}, {}};
    for (int i = 0; i < 400; ++i) {
        const Vec2 input = inputs[(i / 50) % 4];
        horde::StepWorld(a, input, kDt);
        horde::StepWorld(b, input, kDt);
    }

    CHECK(a.player.motion.position == b.player.motion.position);
    for (std::size_t i = 0; i < a.enemies.size(); ++i) {
        CHECK(a.enemies[i].motion.position == b.enemies[i].motion.position);
    }
}
