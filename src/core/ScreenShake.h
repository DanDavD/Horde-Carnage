#pragma once

#include "core/Vec2.h"

namespace horde {

/// How hard and how fast the screen shakes.
struct ScreenShakeStats {
    float maxOffset = 22.0f;     ///< How far the view moves on each axis at full trauma, in world units.
    float maxTurnDegrees = 1.2f; ///< How far the view turns at full trauma.
    float decayPerSecond = 1.5f; ///< Trauma lost per second: full trauma settles in two thirds of a second.
    float frequency = 18.0f;     ///< How many new directions the shake heads for each second.
};

/// A shake built on trauma, a number from 0 to 1 that hits add to and time takes away.
///
/// The shake is trauma squared, so a small hit barely moves the screen while a big one throws it, and two
/// small hits do not add up to one big one. The direction comes from smooth noise rather than a new random
/// number every frame, so the view sways instead of jittering, and it does not depend on the frame rate.
struct ScreenShake {
    float trauma = 0.0f;
    float seconds = 0.0f; ///< Time since the shake started, where the noise is read.
    ScreenShakeStats stats;
};

/// How far the view is moved and turned in this frame.
struct ShakeOffset {
    Vec2 offset;
    float turnDegrees = 0.0f;
};

/// Adds `amount` of trauma, up to 1.
void AddTrauma(ScreenShake& shake, float amount);

/// Takes `dt` seconds of trauma away and moves along the noise.
void StepScreenShake(ScreenShake& shake, float dt);

/// The shake right now: at most `maxOffset` x trauma squared on each axis and `maxTurnDegrees` x trauma
/// squared of turn, and nothing at all without trauma.
[[nodiscard]] ShakeOffset CurrentShake(const ScreenShake& shake);

} // namespace horde
