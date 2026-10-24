#pragma once

#include "core/Health.h"
#include "core/Movement.h"

namespace horde {

/// The Matriarch, the one boss the game is about: a giant hairball, mother of the small ones.
///
/// It has the same body components as the player and the hairballs (`Motion`, `MovementStats`, a radius
/// and a weight), so it moves, collides and stays inside the walls through the same systems. It is a type
/// of its own rather than an `Enemy` with bigger numbers because its rules differ: bullets neither stagger
/// nor push it, and its attacks run on their own state machine.
///
/// A dead boss takes no part in the simulation: it does not chase, block, take bullets or hurt.
struct Boss {
    Motion motion;
    /// Slower than a hairball (160) when it walks, and slow to get going. It is not the walk that makes it
    /// dangerous, it is the attacks.
    MovementStats stats{.maxSpeed = 120.0f, .acceleration = 400.0f, .deceleration = 1500.0f};
    float radius = 60.0f; ///< Three hairballs across.
    /// Ten times the player: the player cannot shove it, and it pushes the player out of its way.
    float weight = 40.0f;
    /// Twice a hairball's touch. Five touches kill a player at full health.
    int contactDamage = 20;
    /// 120 pistol bullets or 200 SMG bullets. No invulnerability window, like the hairballs, so every
    /// bullet counts.
    Health health{.current = 1200, .max = 1200, .invulnerabilityAfterHit = 0.0f};
    float hitFlashSeconds = 0.0f; ///< Time left on the white flash of the last hit.
};

} // namespace horde
