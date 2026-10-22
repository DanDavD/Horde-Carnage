#pragma once

#include "core/Vec2.h"

namespace horde {

/// The movement input of a chaser at `from` that walks straight at `target`: length 1 toward the target,
/// or zero once it is within `stopDistance` of it.
///
/// The result goes into `StepMovement` like the player's input, so a chaser accelerates, brakes and
/// hits walls by the same rules as the player.
///
/// @param stopDistance Usually the sum of both radii, so the chaser stops when the two bodies touch.
[[nodiscard]] Vec2 ChaseInput(Vec2 from, Vec2 target, float stopDistance);

} // namespace horde
