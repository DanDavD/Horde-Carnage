#pragma once

#include <optional>

#include "core/Vec2.h"

namespace horde {

/// Reads where the player wants to move this step, from the keyboard (WASD or arrows) and the
/// left stick and d-pad of the first gamepad. The result can be longer than 1; `StepMovement` clamps it.
[[nodiscard]] Vec2 ReadMoveInput();

/// Whether a restart button went down this frame: Enter or R on the keyboard, A or Start on the first
/// gamepad. Call it once per frame, not once per fixed step: a press is an event, and a frame that runs
/// no step would lose it.
[[nodiscard]] bool ReadRestartPressed();

/// Whether the fire button (the left mouse button) is down right now. A held state, so reading it once
/// per fixed step is right.
[[nodiscard]] bool ReadFireHeld();

/// Button presses that are events, gathered over one or more frames until a fixed step uses them.
struct ButtonPresses {
    bool fire = false;              ///< Left mouse button.
    std::optional<int> weaponSlot;  ///< Keys 1 and 2 pick slots 0 and 1.
    bool cycleWeapon = false;       ///< Q or the mouse wheel.
    bool dash = false;              ///< Space or left Shift.
};

/// Adds this frame's presses to `presses`. Call it once per frame: a frame that runs no fixed step keeps
/// them for the next one, instead of losing them.
void CollectButtonPresses(ButtonPresses& presses);

} // namespace horde
