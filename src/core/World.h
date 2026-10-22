#pragma once

#include <vector>

#include "core/Arena.h"
#include "core/Enemy.h"
#include "core/FollowCamera.h"
#include "core/Player.h"
#include "core/Vec2.h"

namespace horde {

/// How many hairballs a new world starts with, and how far from the player they appear.
/// The wave director on day 5 replaces this fixed ring.
inline constexpr int kStartingHairballs = 6;
inline constexpr float kHairballSpawnDistance = 520.0f;

/// Everything the simulation needs, as plain data: the arena, the player, the enemies and the camera.
///
/// The app reads the input and draws a World. It never changes one directly. Restarting the game
/// will be assigning a fresh World, so nothing from the last run can leak into the next.
struct World {
    Arena arena;
    Player player;
    std::vector<Enemy> enemies;
    FollowCamera camera;
};

/// The starting world: the player in the middle of `arena`, `kStartingHairballs` hairballs on a ring
/// around it, and the camera settled on the player.
///
/// The enemy list is sized here, once. `StepWorld` never adds or removes enemies, so a running game
/// does not allocate memory.
[[nodiscard]] World CreateWorld(const Arena& arena, Vec2 viewSize);

/// Advances the whole simulation by one fixed step of `dt` seconds: everyone moves, then overlapping
/// bodies are pushed apart, then everyone is held inside the arena, then the camera follows.
///
/// @param moveInput The player's movement input for this step, as `ReadMoveInput` gives it.
void StepWorld(World& world, Vec2 moveInput, float dt);

} // namespace horde
