#pragma once

#include "core/Vec2.h"

namespace horde {

/// The numbers of a shockwave.
struct ShockwaveStats {
    float speed = 380.0f;    ///< How fast the ring grows, in units per second. Faster than the player runs.
    float reach = 420.0f;    ///< The radius at which it dies out.
    float thickness = 20.0f; ///< Width of the band that hurts, behind its front edge.
    int damage = 20;
};

/// A ring that grows from where the boss lands. Only its band hurts: the player is safe inside the ring,
/// once it has passed, and outside it, until it arrives. Each wave can hurt the player once.
struct Shockwave {
    Vec2 center;
    float radius = 0.0f; ///< The front edge.
    ShockwaveStats stats;
    bool spent = false; ///< It already hit the player.
};

/// A shockwave starting at `center` with its front at `radius`.
[[nodiscard]] Shockwave MakeShockwave(Vec2 center, float radius, const ShockwaveStats& stats);

/// Grows the ring by one fixed step.
///
/// @return Whether it is still going. False once it has passed its reach.
bool StepShockwave(Shockwave& wave, float dt);

/// Whether a circle at `position` with `radius` overlaps the hurting band of the ring.
[[nodiscard]] bool ShockwaveTouches(const Shockwave& wave, Vec2 position, float radius);

} // namespace horde
