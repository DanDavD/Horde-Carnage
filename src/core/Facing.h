#pragma once

#include "core/Vec2.h"

namespace horde {

/// Turns `facing` toward the direction of `input`. With no input it keeps the old direction,
/// so a player who stops still knows which way to swing.
///
/// @param facing Always length 1.
/// @param input A movement input, of any length. Only its direction is used, so a stick barely
///        past the deadzone turns the player as fully as a stick at full tilt.
void UpdateFacing(Vec2& facing, Vec2 input);

} // namespace horde
