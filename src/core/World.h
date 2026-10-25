#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "core/Arena.h"
#include "core/Boss.h"
#include "core/DamageSource.h"
#include "core/Enemy.h"
#include "core/FollowCamera.h"
#include "core/ObjectPool.h"
#include "core/Player.h"
#include "core/PlayerCommands.h"
#include "core/Projectile.h"
#include "core/Random.h"
#include "core/Shockwave.h"
#include "core/Vec2.h"

namespace horde {

/// The room of the fight. Wider than the screen, so there is room to run and to dodge across it, and
/// small enough that the camera can keep the boss in view from almost anywhere.
inline constexpr Arena kBossArena{{0.0f, 0.0f}, {1640.0f, 1040.0f}};

/// How far south of the boss the player starts. Far enough to see the boss coming, close enough that both
/// start on screen.
inline constexpr float kStartingDistance = 400.0f;

/// Pool sizes. The SMG keeps about seven bullets in the air at once, and 64 hairballs is far more than
/// the boss will ever summon. A Hair Ring is 42 strands that fly for up to four seconds, so 128 holds
/// three rings at once. A shockwave lasts about a second, so 8 is room for the second phase's leaps too.
inline constexpr std::size_t kMaxEnemies = 64;
inline constexpr std::size_t kMaxBullets = 128;
inline constexpr std::size_t kMaxHostileProjectiles = 128;
inline constexpr std::size_t kMaxShockwaves = 8;

/// Everything the simulation needs, as plain data: the arena, the player, the boss, the hairballs, the
/// bullets, the boss's projectiles and shockwaves, the camera, and the random generator.
///
/// The app reads the input and draws a World. It never changes one directly. Restarting the game is
/// assigning a fresh World (see `Session`), so nothing from the last run can leak into the next. Hairballs,
/// projectiles and shockwaves live in fixed pools inside the World, so a running game never allocates
/// memory. Everything random draws from `random`, so the seed and the inputs decide the whole fight.
struct World {
    Arena arena;
    Player player;
    Boss boss;
    /// Hairballs. The fight starts without any; the pool is there for the ones the boss summons.
    ObjectPool<Enemy, kMaxEnemies> enemies;
    ObjectPool<Projectile, kMaxBullets> bullets; ///< The player's. They hit hairballs and the boss.
    /// The boss's: the strands of its Hair Ring. They hit only the player. A pool of their own keeps the two
    /// sides apart without a flag on every projectile.
    ObjectPool<Projectile, kMaxHostileProjectiles> hostileProjectiles;
    ObjectPool<Shockwave, kMaxShockwaves> shockwaves; ///< From the boss's landings. They hit only the player.
    FollowCamera camera;
    std::uint64_t seed = 0; ///< What `random` was seeded with, so a fight can be told apart and replayed.
    Random random;
    float elapsedSeconds = 0.0f; ///< Simulated time since the world was created: how long the fight has lasted.
    int shotsFired = 0;          ///< Shots fired this run.
    int shotsHit = 0;            ///< Of those, the ones that hit the boss or a hairball.
    /// What landed the last hit on the player, and so, once the player is dead, what killed them. Empty
    /// until the first hit.
    std::optional<DamageSource> lastHitBy;
};

/// The starting world: the boss in the middle of `arena`, the player `kStartingDistance` south of it and
/// facing it, no hairballs, the camera settled on both, and the random generator seeded with `seed`.
/// Tests that do not depend on chance leave the seed at its default.
[[nodiscard]] World CreateWorld(const Arena& arena, Vec2 viewSize, std::uint64_t seed = 0);

/// Advances the whole simulation by one fixed step of `dt` seconds, in this order: the player moves,
/// aims and fires; hairballs chase and the boss stalks or attacks, spitting its strands into their pool
/// or landing from a leap; bodies are pushed apart and held inside the arena; bullets fly and hit; dead
/// hairballs are removed; bodies that touch the player hurt it; the boss's strands and shockwaves move and
/// hit the player; and the camera follows.
///
/// @param commands What the player asks for in this step: movement, aim, fire and weapon switching.
void StepWorld(World& world, const PlayerCommands& commands, float dt);

/// Whether the fight is won: the boss's health reaching 0 in its second phase. In the first phase an empty
/// bar starts the transformation instead.
[[nodiscard]] bool IsBossDefeated(const World& world);

/// The share of shots this run that hit something, from 0 to 1. 0 when nothing was fired.
[[nodiscard]] float Accuracy(const World& world);

/// Takes all the boss's health at once, in its second phase. For the Debug shortcut that skips to the
/// victory screen.
void DefeatBoss(World& world);

/// Breaks the boss's shield at once, staggering it as a real break would. For the Debug shortcut that skips
/// to the exposed window without the right gun.
void BreakBossShield(World& world);

/// Starts the boss's transformation into its second phase at once. For the Debug shortcut that skips the
/// first phase.
void TransformBoss(World& world);

/// In the second phase, drops the boss to half its health and throws it into its frenzy at once. For the
/// Debug shortcut that skips to the end of the fight.
void FrenzyBoss(World& world);

} // namespace horde
