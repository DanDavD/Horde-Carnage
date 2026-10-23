#pragma once

#include <cstddef>

#include "core/Arena.h"
#include "core/Enemy.h"
#include "core/FollowCamera.h"
#include "core/ObjectPool.h"
#include "core/Player.h"
#include "core/PlayerCommands.h"
#include "core/Projectile.h"
#include "core/Vec2.h"

namespace horde {

/// How many hairballs a ring has, and how far from the player they appear. The wave director on day 5
/// replaces the ring.
inline constexpr int kStartingHairballs = 6;
inline constexpr float kHairballSpawnDistance = 520.0f;

/// Once the last hairball dies, a new ring appears this long after, so the arena never stays empty.
inline constexpr float kRespawnDelaySeconds = 2.0f;

/// Pool sizes. 64 enemies is well above what the waves of day 5 plan for, and the SMG keeps about seven
/// bullets in the air at once.
inline constexpr std::size_t kMaxEnemies = 64;
inline constexpr std::size_t kMaxBullets = 128;

/// Everything the simulation needs, as plain data: the arena, the player, the enemies, the bullets and
/// the camera.
///
/// The app reads the input and draws a World. It never changes one directly. Restarting the game is
/// assigning a fresh World (see `Session`), so nothing from the last run can leak into the next. Enemies
/// and bullets live in fixed pools inside the World, so a running game never allocates memory.
struct World {
    Arena arena;
    Player player;
    ObjectPool<Enemy, kMaxEnemies> enemies;
    ObjectPool<Projectile, kMaxBullets> bullets;
    FollowCamera camera;
    float elapsedSeconds = 0.0f; ///< Simulated time since the world was created: how long the player survived.
    float clearedSeconds = 0.0f; ///< How long the arena has been empty of enemies.
    int shotsFired = 0;          ///< Shots fired this run. Later the game over screen can show accuracy.
    int kills = 0;               ///< Enemies killed this run.
};

/// The starting world: the player in the middle of `arena`, a ring of hairballs around it, and the camera
/// settled on the player.
[[nodiscard]] World CreateWorld(const Arena& arena, Vec2 viewSize);

/// Adds `kStartingHairballs` hairballs on a ring of `kHairballSpawnDistance` around the player. Hairballs
/// that do not fit in the pool are not added.
void SpawnHairballRing(World& world);

/// Advances the whole simulation by one fixed step of `dt` seconds, in this order: the player moves,
/// aims and fires; enemies chase; bodies are pushed apart and held inside the arena; bullets fly and hit;
/// dead enemies are removed; hairballs that touch the player hurt it; an empty arena is refilled; and
/// the camera follows.
///
/// @param commands What the player asks for in this step: movement, aim, fire and weapon switching.
void StepWorld(World& world, const PlayerCommands& commands, float dt);

} // namespace horde
