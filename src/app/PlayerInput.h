#pragma once

#include "core/Vec2.h"

namespace horde {

/// Reads where the player wants to move this step, from the keyboard (WASD or arrows) and the
/// left stick and d-pad of the first gamepad. The result can be longer than 1; `StepMovement` clamps it.
[[nodiscard]] Vec2 ReadMoveInput();

/// Whether a restart button went down this frame: Enter or R on the keyboard, A or Start on the first
/// gamepad. Call it once per frame, not once per fixed step: a press is an event, and a frame that runs
/// no step would lose it.
[[nodiscard]] bool ReadRestartPressed();

} // namespace horde
