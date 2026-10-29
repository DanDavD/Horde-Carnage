#pragma once

#include <optional>

#include "core/Vec2.h"

namespace horde {

/// What the player asks for in one fixed step, already translated from the hardware by the app.
///
/// The simulation never reads a key or the mouse itself. Tests drive it with plain values, and a
/// gamepad will fill the same fields later without the rules knowing the difference.
struct PlayerCommands {
    Vec2 move; ///< Movement input. Longer than 1 is fine; `StepMovement` clamps it.
    /// World point to aim at. Empty when another aim field or movement decides the facing.
    std::optional<Vec2> aimAt;
    /// Offset from the camera's center in world units, resolved after movement and collisions so a screen
    /// crosshair never lags a step behind a strafing player or look-ahead camera.
    std::optional<Vec2> aimInView;
    /// Unit aim direction from the player, used by twin-stick aiming so moving sideways never skews the shot.
    std::optional<Vec2> aimDirection;
    bool fireHeld = false;    ///< The fire button is down during this step.
    bool firePressed = false; ///< The fire button went down since the last step.
    std::optional<int> selectSlot; ///< A weapon slot picked directly, counting from 0.
    bool cycleWeapon = false;      ///< Switch to the next weapon.
    bool dashPressed = false;      ///< The dash button went down since the last step.
    bool healPressed = false;      ///< The heal button went down since the last step.
};

} // namespace horde
