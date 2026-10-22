#include "core/World.h"

#include <cmath>
#include <numbers>

#include "core/Chase.h"
#include "core/Facing.h"
#include "core/Movement.h"

namespace horde {

World CreateWorld(const Arena& arena, Vec2 viewSize) {
    World world{.arena = arena, .player = {.motion = MotionAt(Center(arena))}};

    // Evenly spaced on a ring, so the hairballs come from every side at once and some start off screen.
    const Vec2 center = world.player.motion.position;
    world.enemies.reserve(kStartingHairballs);
    for (int i = 0; i < kStartingHairballs; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / kStartingHairballs;
        const Vec2 offset{std::cos(angle) * kHairballSpawnDistance, std::sin(angle) * kHairballSpawnDistance};
        world.enemies.push_back(Enemy{.motion = MotionAt(center + offset)});
    }

    world.camera = FollowCameraAt(center, viewSize, arena);
    return world;
}

void StepWorld(World& world, Vec2 moveInput, float dt) {
    Player& player = world.player;
    UpdateFacing(player.facing, moveInput);
    StepMovement(player.motion, moveInput, player.stats, dt);
    ConfineToArena(player.motion, player.radius, world.arena);

    // Enemies chase where the player is after this step's move, so they never aim at a stale position.
    for (Enemy& enemy : world.enemies) {
        const Vec2 input = ChaseInput(enemy.motion.position, player.motion.position, enemy.radius + player.radius);
        StepMovement(enemy.motion, input, enemy.stats, dt);
        ConfineToArena(enemy.motion, enemy.radius, world.arena);
    }

    // Last, so the camera follows where everything ended up in this step.
    StepFollowCamera(world.camera, player.motion.position, player.motion.velocity, world.arena, dt);
}

} // namespace horde
