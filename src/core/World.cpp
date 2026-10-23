#include "core/World.h"

#include <cmath>
#include <cstddef>
#include <numbers>

#include "core/Chase.h"
#include "core/Collision.h"
#include "core/Facing.h"
#include "core/Health.h"
#include "core/Loadout.h"
#include "core/Movement.h"

namespace horde {

namespace {

// Pushing one pair apart can push one of them into a third body. A few passes over all pairs settle a
// crowd. Each pass ends with the walls, so a body pushed into a wall is put back before the next pass.
constexpr int kCollisionPasses = 4;

void ConfineEveryone(World& world) {
    ConfineToArena(world.player.motion, world.player.radius, world.arena);
    for (Enemy& enemy : world.enemies) {
        ConfineToArena(enemy.motion, enemy.radius, world.arena);
    }
}

// Every pair once: the player against each enemy, then each enemy against the ones after it. With a few
// dozen bodies that is a few hundred checks per pass, so a spatial grid would cost more than it saves.
void SeparateEveryone(World& world) {
    Player& player = world.player;
    std::vector<Enemy>& enemies = world.enemies;
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        Enemy& enemy = enemies[i];
        SeparateCircles(player.motion, player.radius, enemy.motion, enemy.radius,
                        PushShare(player.weight, enemy.weight));
        for (std::size_t j = i + 1; j < enemies.size(); ++j) {
            Enemy& other = enemies[j];
            SeparateCircles(enemy.motion, enemy.radius, other.motion, other.radius,
                            PushShare(enemy.weight, other.weight));
        }
    }
}

// After collisions, so contact means the bodies really ended the step touching. The invulnerability
// window lets only the first touch of a crowd land.
void ApplyContactDamage(World& world) {
    Player& player = world.player;
    for (const Enemy& enemy : world.enemies) {
        if (AreTouching(player.motion.position, player.radius, enemy.motion.position, enemy.radius)) {
            ApplyDamage(player.health, enemy.contactDamage);
        }
    }
}

// Switching happens before the trigger, so a switch and a click in the same step fire the new gun.
void HandleWeapons(Player& player, const PlayerCommands& commands, float dt, int& shotsFired) {
    Loadout& loadout = player.loadout;
    if (commands.selectSlot) {
        SelectSlot(loadout, *commands.selectSlot);
    }
    if (commands.cycleWeapon) {
        CycleWeapon(loadout);
    }
    TickLoadout(loadout, dt);
    if (PullTrigger(Equipped(loadout), commands.fireHeld, commands.firePressed)) {
        // The bullet itself comes with the projectile pool. For now a shot is the flash and this count.
        ++shotsFired;
    }
}

} // namespace

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

void StepWorld(World& world, const PlayerCommands& commands, float dt) {
    world.elapsedSeconds += dt;
    Player& player = world.player;
    TickHealth(player.health, dt);
    StepMovement(player.motion, commands.move, player.stats, dt);

    // Aim after moving, so the gun points from where the player is now, not from a step ago.
    if (commands.aimAt) {
        AimFacing(player.facing, player.motion.position, *commands.aimAt);
    } else {
        UpdateFacing(player.facing, commands.move);
    }
    HandleWeapons(player, commands, dt, world.shotsFired);

    // Enemies chase where the player is after this step's move, so they never aim at a stale position.
    for (Enemy& enemy : world.enemies) {
        const float stopDistance = enemy.radius + player.radius + kContactSlack;
        const Vec2 input = ChaseInput(enemy.motion.position, player.motion.position, stopDistance);
        StepMovement(enemy.motion, input, enemy.stats, dt);
    }

    // Collisions run after everyone has moved, so the order in which bodies moved does not decide who
    // gets pushed.
    for (int pass = 0; pass < kCollisionPasses; ++pass) {
        SeparateEveryone(world);
        ConfineEveryone(world);
    }

    ApplyContactDamage(world);

    // Last, so the camera follows where everything ended up in this step.
    StepFollowCamera(world.camera, player.motion.position, player.motion.velocity, world.arena, dt);
}

} // namespace horde
