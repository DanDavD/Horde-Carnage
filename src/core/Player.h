#pragma once

#include "core/Dash.h"
#include "core/Flasks.h"
#include "core/Health.h"
#include "core/Loadout.h"
#include "core/Movement.h"

namespace horde {

/// The player's data. Behaviour lives in systems (`StepMovement`, `ConfineToArena`, `UpdateFacing`,
/// `SeparateCircles`), not here, so enemies can reuse the same systems with their own data.
struct Player {
    Motion motion;
    MovementStats stats;
    float radius = 20.0f; ///< Body size in world units, for walls, collisions and contact damage.
    /// How hard the player is to push, against an enemy's `weight`. Four times a hairball: one hairball
    /// cannot pin the player, but a crowd slows them down.
    float weight = 4.0f;
    /// Direction the player looks and aims, length 1. It follows the mouse, not the movement, so the
    /// player can run one way and shoot another. Starts facing down, toward the camera.
    Vec2 facing{0.0f, 1.0f};
    Health health;
    Loadout loadout;
    Dash dash;
    Flasks flasks;
};

} // namespace horde
