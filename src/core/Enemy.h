#pragma once

#include "core/Movement.h"

namespace horde {

/// An enemy's data. For now every enemy is a hairball: it walks straight at the player.
///
/// It has the same components as the player, so it moves through the same systems (`StepMovement`,
/// `ConfineToArena`). Only the numbers differ.
struct Enemy {
    Motion motion;
    /// Slower than the player (300), so the player can always outrun a hairball. The slow start reads
    /// as a lump of hair getting rolling, and the sharp stop keeps it from sliding into the player.
    MovementStats stats{.maxSpeed = 160.0f, .acceleration = 600.0f, .deceleration = 2000.0f};
    float radius = 18.0f; ///< Body size in world units.
};

} // namespace horde
