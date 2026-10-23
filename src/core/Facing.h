#pragma once

#include "core/Vec2.h"

namespace horde {

/// Turns `facing` toward the direction of `input`. With no input it keeps the old direction.
///
/// This is the fallback when nothing aims (no mouse, and later a gamepad without the right stick pushed):
/// the player looks where it walks.
///
/// @param facing Always length 1.
/// @param input A movement input, of any length. Only its direction is used, so a stick barely
///        past the deadzone turns the player as fully as a stick at full tilt.
void UpdateFacing(Vec2& facing, Vec2 input);

/// Turns `facing` from `from` toward `target`, the point the player aims at. If the target is on top of
/// `from` there is no direction, and the old facing is kept.
void AimFacing(Vec2& facing, Vec2 from, Vec2 target);

} // namespace horde
