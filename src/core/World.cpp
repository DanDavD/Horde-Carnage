#include "core/World.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

#include "core/Chase.h"
#include "core/Collision.h"
#include "core/Dash.h"
#include "core/Facing.h"
#include "core/Health.h"
#include "core/Loadout.h"
#include "core/Movement.h"

namespace horde {

namespace {

// Pushing one pair apart can push one of them into a third body. A few passes over all pairs settle a
// crowd. Each pass ends with the walls, so a body pushed into a wall is put back before the next pass.
constexpr int kCollisionPasses = 4;

// A hit flashes the hairball white, and stops it chasing for a moment so the hit reads as a stagger.
constexpr float kHitFlashSeconds = 0.1f;
constexpr float kHitStunSeconds = 0.15f;

// Every source of damage to the player goes through here, so a dash protects from all of them.
void HurtPlayer(Player& player, int damage) {
    if (!IsDashInvulnerable(player.dash)) {
        ApplyDamage(player.health, damage);
    }
}

// A dash goes where the player is pushing. Standing still, it goes toward the aim point of this step, not
// the facing: the facing is only updated after moving, so it is a step old, and a flick of the mouse
// followed by a dash in the same frame would go the wrong way.
void StartDashIfAsked(Player& player, const PlayerCommands& commands) {
    if (!commands.dashPressed || IsDashing(player.dash)) {
        return;
    }
    // With the cursor right on the player there is no aim direction (the same one unit tolerance as
    // AimFacing), and the facing is the best guess.
    Vec2 direction = player.facing;
    if (LengthSquared(commands.move) > 0.0f) {
        direction = commands.move;
    } else if (commands.aimAt && LengthSquared(*commands.aimAt - player.motion.position) > 1.0f) {
        direction = *commands.aimAt - player.motion.position;
    }
    TryStartDash(player.dash, player.motion.position, direction);
}

void MovePlayer(Player& player, Vec2 move, float dt) {
    Dash& dash = player.dash;
    const bool dashing = IsDashing(dash);
    const Vec2 dashStep = StepDash(dash, dt);
    if (!dashing) {
        StepMovement(player.motion, move, player.stats, dt);
        return;
    }
    // The burst replaces normal movement. The velocity reports running speed rather than the burst's
    // 1500 units/s: the player leaves the dash at running pace instead of sliding on, and the camera's
    // look-ahead does not lurch forward for a tenth of a second.
    player.motion.previousPosition = player.motion.position;
    player.motion.position += dashStep;
    player.motion.velocity = dash.direction * player.stats.maxSpeed;
}

// Switching happens before the trigger, so a switch and a click in the same step fire the new gun.
void HandleWeapons(World& world, const PlayerCommands& commands, float dt) {
    Player& player = world.player;
    Loadout& loadout = player.loadout;
    if (commands.selectSlot) {
        SelectSlot(loadout, *commands.selectSlot);
    }
    if (commands.cycleWeapon) {
        CycleWeapon(loadout);
    }
    TickLoadout(loadout, dt);

    Weapon& weapon = Equipped(loadout);
    const bool handsFree = !IsDashing(player.dash);
    if (PullTrigger(weapon, commands.fireHeld, commands.firePressed, handsFree)) {
        const Vec2 muzzle = MuzzlePoint(player.motion.position, player.facing, player.radius, weapon.stats);
        world.bullets.Spawn(MakeBullet(muzzle, player.facing, weapon.stats));
        ++world.shotsFired;
    }
}

void StepEnemies(World& world, float dt) {
    const Player& player = world.player;
    for (Enemy& enemy : world.enemies) {
        enemy.hitFlashSeconds = std::max(enemy.hitFlashSeconds - dt, 0.0f);
        enemy.stunSeconds = std::max(enemy.stunSeconds - dt, 0.0f);

        // Chase where the player is after this step's move, so a hairball never aims at a stale position.
        // While stunned it has no input: it slides out of the knockback and holds still, instead of turning
        // straight back toward the player.
        const float stopDistance = enemy.radius + player.radius + kContactSlack;
        const Vec2 input = enemy.stunSeconds > 0.0f
                               ? Vec2{}
                               : ChaseInput(enemy.motion.position, player.motion.position, stopDistance);
        StepMovement(enemy.motion, input, enemy.stats, dt);
    }
}

void ConfineEveryone(World& world) {
    ConfineToArena(world.player.motion, world.player.radius, world.arena);
    for (Enemy& enemy : world.enemies) {
        ConfineToArena(enemy.motion, enemy.radius, world.arena);
    }
}

// Every pair once: the player against each enemy, then each enemy against the ones after it. With a few
// dozen bodies that is a few hundred checks per pass, so a spatial grid would cost more than it saves.
// A dashing player passes through bodies, so a dash can escape a crowd or go through the boss.
void SeparateEveryone(World& world) {
    Player& player = world.player;
    const bool playerIsSolid = !IsDashing(player.dash);
    auto& enemies = world.enemies;
    for (std::size_t i = 0; i < enemies.Size(); ++i) {
        Enemy& enemy = enemies[i];
        if (playerIsSolid) {
            SeparateCircles(player.motion, player.radius, enemy.motion, enemy.radius,
                            PushShare(player.weight, enemy.weight));
        }
        for (std::size_t j = i + 1; j < enemies.Size(); ++j) {
            Enemy& other = enemies[j];
            SeparateCircles(enemy.motion, enemy.radius, other.motion, other.radius,
                            PushShare(enemy.weight, other.weight));
        }
    }
}

// The enemy the bullet touches first along this step's flight, if any. Comparing how far along the path
// each hit happens is what makes the front hairball of a line take the bullet.
Enemy* FirstEnemyHit(World& world, const Projectile& bullet) {
    Enemy* first = nullptr;
    float firstFraction = 2.0f;
    for (Enemy& enemy : world.enemies) {
        // A hairball killed by an earlier bullet this step is removed only after all bullets have flown.
        // Until then it must not soak up more bullets.
        if (IsDead(enemy.health)) {
            continue;
        }
        const std::optional<float> fraction = SweepCircle(bullet.motion.previousPosition, bullet.motion.position,
                                                          bullet.radius, enemy.motion.position, enemy.radius);
        if (fraction && *fraction < firstFraction) {
            first = &enemy;
            firstFraction = *fraction;
        }
    }
    return first;
}

void HitEnemy(Enemy& enemy, const Projectile& bullet) {
    ApplyDamage(enemy.health, bullet.damage);
    enemy.hitFlashSeconds = kHitFlashSeconds;
    enemy.stunSeconds = kHitStunSeconds;
    enemy.motion.velocity += Normalized(bullet.motion.velocity) * bullet.knockback;
}

// One pass moves every bullet, resolves its hit and removes the ones that are done, so a bullet is never
// checked in a step it no longer flies in.
void StepBullets(World& world, float dt) {
    world.bullets.RemoveIf([&world, dt](Projectile& bullet) {
        const bool stillFlying = StepProjectile(bullet, dt);
        if (Enemy* enemy = FirstEnemyHit(world, bullet)) {
            HitEnemy(*enemy, bullet);
            return true;
        }
        return !stillFlying || !Contains(world.arena, bullet.motion.position);
    });
}

void RemoveDeadEnemies(World& world) {
    const std::size_t dead = world.enemies.RemoveIf([](const Enemy& enemy) { return IsDead(enemy.health); });
    world.kills += static_cast<int>(dead);
}

// After collisions, so contact means the bodies really ended the step touching. The invulnerability
// window lets only the first touch of a crowd land.
void ApplyContactDamage(World& world) {
    Player& player = world.player;
    for (const Enemy& enemy : world.enemies) {
        if (AreTouching(player.motion.position, player.radius, enemy.motion.position, enemy.radius)) {
            HurtPlayer(player, enemy.contactDamage);
        }
    }
}

// Until the wave director on day 5, a cleared arena gets a new ring after a short breather.
void RefillEmptyArena(World& world, float dt) {
    if (!world.enemies.Empty()) {
        world.clearedSeconds = 0.0f;
        return;
    }
    world.clearedSeconds += dt;
    if (world.clearedSeconds >= kRespawnDelaySeconds) {
        SpawnHairballRing(world);
        world.clearedSeconds = 0.0f;
    }
}

} // namespace

World CreateWorld(const Arena& arena, Vec2 viewSize) {
    World world{.arena = arena, .player = {.motion = MotionAt(Center(arena))}};
    SpawnHairballRing(world);
    world.camera = FollowCameraAt(world.player.motion.position, viewSize, arena);
    return world;
}

void SpawnHairballRing(World& world) {
    // Evenly spaced on a ring, so the hairballs come from every side at once and some start off screen.
    // Near a wall part of the ring falls outside the arena; the walls put those hairballs back inside.
    const Vec2 center = world.player.motion.position;
    for (int i = 0; i < kStartingHairballs; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / kStartingHairballs;
        const Vec2 offset{std::cos(angle) * kHairballSpawnDistance, std::sin(angle) * kHairballSpawnDistance};
        world.enemies.Spawn(Enemy{.motion = MotionAt(center + offset)});
    }
}

void StepWorld(World& world, const PlayerCommands& commands, float dt) {
    world.elapsedSeconds += dt;
    Player& player = world.player;
    TickHealth(player.health, dt);
    TickDashCooldown(player.dash, dt);
    StartDashIfAsked(player, commands);
    MovePlayer(player, commands.move, dt);

    // Aim after moving, so the gun points from where the player is now, not from a step ago.
    if (commands.aimAt) {
        AimFacing(player.facing, player.motion.position, *commands.aimAt);
    } else {
        UpdateFacing(player.facing, commands.move);
    }
    HandleWeapons(world, commands, dt);
    StepEnemies(world, dt);

    // Collisions run after everyone has moved, so the order in which bodies moved does not decide who
    // gets pushed.
    for (int pass = 0; pass < kCollisionPasses; ++pass) {
        SeparateEveryone(world);
        ConfineEveryone(world);
    }

    // Bullets test against where the bodies ended this step. A hairball killed now is removed before it
    // can touch the player.
    StepBullets(world, dt);
    RemoveDeadEnemies(world);
    ApplyContactDamage(world);
    RefillEmptyArena(world, dt);

    // Last, so the camera follows where everything ended up in this step.
    StepFollowCamera(world.camera, player.motion.position, player.motion.velocity, world.arena, dt);
}

} // namespace horde
