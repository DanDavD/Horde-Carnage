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
    /// World point to aim at. Empty when no device is aiming; then the player faces where it moves.
    std::optional<Vec2> aimAt;
    bool fireHeld = false;    ///< The fire button is down during this step.
    bool firePressed = false; ///< The fire button went down since the last step.
    std::optional<int> selectSlot; ///< A weapon slot picked directly, counting from 0.
    bool cycleWeapon = false;      ///< Switch to the next weapon.
};

} // namespace horde
