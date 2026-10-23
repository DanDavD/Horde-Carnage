#pragma once

#include <vector>

#include "core/Arena.h"
#include "core/Enemy.h"
#include "core/FollowCamera.h"
#include "core/Player.h"
#include "core/PlayerCommands.h"
#include "core/Vec2.h"

namespace horde {

/// How many hairballs a new world starts with, and how far from the player they appear.
/// The wave director on day 5 replaces this fixed ring.
inline constexpr int kStartingHairballs = 6;
inline constexpr float kHairballSpawnDistance = 520.0f;

/// Everything the simulation needs, as plain data: the arena, the player, the enemies and the camera.
///
/// The app reads the input and draws a World. It never changes one directly. Restarting the game is
/// assigning a fresh World (see `Session`), so nothing from the last run can leak into the next.
struct World {
    Arena arena;
    Player player;
    std::vector<Enemy> enemies;
    FollowCamera camera;
    float elapsedSeconds = 0.0f; ///< Simulated time since the world was created: how long the player survived.
    int shotsFired = 0;          ///< Shots fired this run. Later the game over screen can show accuracy.
};

/// The starting world: the player in the middle of `arena`, `kStartingHairballs` hairballs on a ring
/// around it, and the camera settled on the player.
///
/// The enemy list is sized here, once. `StepWorld` never adds or removes enemies, so a running game
/// does not allocate memory.
[[nodiscard]] World CreateWorld(const Arena& arena, Vec2 viewSize);

/// Advances the whole simulation by one fixed step of `dt` seconds: everyone moves, the player aims and
/// handles its guns, overlapping bodies are pushed apart, everyone is held inside the arena, hairballs
/// that touch the player hurt it, and the camera follows.
///
/// @param commands What the player asks for in this step: movement, aim, fire and weapon switching.
void StepWorld(World& world, const PlayerCommands& commands, float dt);

} // namespace horde
