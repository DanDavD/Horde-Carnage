#pragma once

#include "core/Movement.h"

namespace horde {

/// The player's data. Behaviour lives in systems (`StepMovement`, `ConfineToArena`, `UpdateFacing`),
/// not here, so enemies can reuse the same systems with their own data.
struct Player {
    Motion motion;
    MovementStats stats;
    float radius = 20.0f; ///< Body size in world units, used for walls now and for hits later.
    /// Direction the player looks, length 1. Starts facing down, toward the camera. The melee swing
    /// on day 4 goes this way.
    Vec2 facing{0.0f, 1.0f};
};

} // namespace horde
