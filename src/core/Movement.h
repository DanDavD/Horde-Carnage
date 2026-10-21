#pragma once

#include "core/Vec2.h"

namespace horde {

/// How fast something moves and how quickly it gets to that speed.
///
/// The defaults are the player's: top speed in 0.1 s and a full stop in 0.075 s. That is close to
/// instant, which is what an action game needs, but the short ramp still reads as weight on screen.
struct MovementStats {
    float maxSpeed = 300.0f;      ///< World units per second.
    float acceleration = 3000.0f; ///< Units per second squared while there is input.
    float deceleration = 4000.0f; ///< Units per second squared when braking: no input, or input against the velocity.
};

/// Where something is and how it moves. The component every moving entity has.
struct Motion {
    Vec2 position;
    /// Position before the last fixed step. The renderer interpolates between this and `position`.
    Vec2 previousPosition;
    Vec2 velocity;
};

/// A mover standing still at `position`. Spawning through this sets `previousPosition` too,
/// so the first interpolated frame is not drawn from the origin.
[[nodiscard]] constexpr Motion MotionAt(Vec2 position) {
    return Motion{.position = position, .previousPosition = position};
}

/// Advances `motion` by one fixed step of `dt` seconds.
///
/// @param input Where the mover wants to go. Its length is the fraction of top speed (an analog
///        stick pushed halfway is 0.5). Anything longer than 1 is clamped to 1.
void StepMovement(Motion& motion, Vec2 input, const MovementStats& stats, float dt);

} // namespace horde
