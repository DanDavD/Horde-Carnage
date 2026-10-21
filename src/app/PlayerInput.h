#pragma once

#include "core/Vec2.h"

namespace horde {

/// Reads where the player wants to move this step, from the keyboard (WASD or arrows) and the
/// left stick of the first gamepad. The result can be longer than 1; `StepMovement` clamps it.
[[nodiscard]] Vec2 ReadMoveInput();

} // namespace horde
