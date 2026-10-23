#pragma once

#include "core/Health.h"
#include "core/Movement.h"

namespace horde {

/// An enemy's data. For now every enemy is a hairball: it walks straight at the player.
///
/// It has the same components as the player, so it moves through the same systems (`StepMovement`,
/// `ConfineToArena`, `SeparateCircles`). Only the numbers differ.
struct Enemy {
    Motion motion;
    /// Slower than the player (300), so the player can always outrun a hairball. The slow start reads
    /// as a lump of hair getting rolling, and the sharp stop keeps it from sliding into the player.
    MovementStats stats{.maxSpeed = 160.0f, .acceleration = 600.0f, .deceleration = 2000.0f};
    float radius = 18.0f; ///< Body size in world units.
    float weight = 1.0f;  ///< How hard it is to push. Only the ratio to the other body's weight matters.
    /// Hit points taken from the player on touch. Ten touches kill a player at full health, and the
    /// player's invulnerability window spaces them out to at most one every 0.8 s.
    int contactDamage = 10;
    /// Three pistol bullets or five SMG bullets. No invulnerability window: every bullet counts, and a
    /// stream of SMG fire must not be cut down to one hit every 0.8 s.
    Health health{.current = 30, .max = 30, .invulnerabilityAfterHit = 0.0f};
    float hitFlashSeconds = 0.0f; ///< Time left on the white flash of the last hit.
    /// Time left in which it does not chase, so a hit staggers it before it comes back.
    float stunSeconds = 0.0f;
};

} // namespace horde
