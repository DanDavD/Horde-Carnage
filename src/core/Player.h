#pragma once

#include "core/Movement.h"

namespace horde {

/// The player's data. Behaviour lives in systems (`StepMovement`, `ConfineToArena`), not here,
/// so enemies can reuse the same systems with their own data.
struct Player {
    Motion motion;
    MovementStats stats;
    float radius = 20.0f; ///< Body size in world units, used for walls now and for hits later.
};

} // namespace horde
