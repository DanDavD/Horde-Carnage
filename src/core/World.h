#pragma once

#include <cstddef>

#include "core/Arena.h"
#include "core/Boss.h"
#include "core/Enemy.h"
#include "core/FollowCamera.h"
#include "core/ObjectPool.h"
#include "core/Player.h"
#include "core/PlayerCommands.h"
#include "core/Projectile.h"
#include "core/Vec2.h"

namespace horde {

/// The room of the fight. Wider than the screen, so there is room to run and to dodge across it, and
/// small enough that the camera can keep the boss in view from almost anywhere.
inline constexpr Arena kBossArena{{0.0f, 0.0f}, {1640.0f, 1040.0f}};

/// How far south of the boss the player starts. Far enough to see the boss coming, close enough that both
/// start on screen.
inline constexpr float kStartingDistance = 400.0f;

/// Pool sizes. The SMG keeps about seven bullets in the air at once, and 64 hairballs is far more than
/// the boss will ever summon.
inline constexpr std::size_t kMaxEnemies = 64;
inline constexpr std::size_t kMaxBullets = 128;

/// Everything the simulation needs, as plain data: the arena, the player, the boss, the hairballs, the
/// bullets and the camera.
///
/// The app reads the input and draws a World. It never changes one directly. Restarting the game is
/// assigning a fresh World (see `Session`), so nothing from the last run can leak into the next. Hairballs
/// and bullets live in fixed pools inside the World, so a running game never allocates memory.
struct World {
    Arena arena;
    Player player;
    Boss boss;
    /// Hairballs. The fight starts without any; the pool is there for the ones the boss summons.
    ObjectPool<Enemy, kMaxEnemies> enemies;
    ObjectPool<Projectile, kMaxBullets> bullets;
    FollowCamera camera;
    float elapsedSeconds = 0.0f; ///< Simulated time since the world was created: how long the fight has lasted.
    int shotsFired = 0;          ///< Shots fired this run.
    int shotsHit = 0;            ///< Of those, the ones that hit the boss or a hairball.
};

/// The starting world: the boss in the middle of `arena`, the player `kStartingDistance` south of it and
/// facing it, no hairballs, and the camera settled on both.
[[nodiscard]] World CreateWorld(const Arena& arena, Vec2 viewSize);

/// Advances the whole simulation by one fixed step of `dt` seconds, in this order: the player moves,
/// aims and fires; hairballs chase and the boss stalks or attacks; bodies are pushed apart and held
/// inside the arena; bullets fly and hit; dead hairballs are removed; bodies that touch the player hurt
/// it; and the camera follows.

///
/// @param commands What the player asks for in this step: movement, aim, fire and weapon switching.
void StepWorld(World& world, const PlayerCommands& commands, float dt);

/// Whether the fight is won. For now that is the boss's health reaching 0; once the boss has a second
/// phase, it will mean the end of that phase, and only this function will change.
[[nodiscard]] bool IsBossDefeated(const World& world);

/// The share of shots this run that hit something, from 0 to 1. 0 when nothing was fired.
[[nodiscard]] float Accuracy(const World& world);

/// Takes all the boss's health at once. For the Debug shortcut that skips to the victory screen.
void DefeatBoss(World& world);

} // namespace horde
