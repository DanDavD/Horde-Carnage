#pragma once

#include "core/Movement.h"

namespace horde {

/// Two bodies this close or closer count as touching. Collisions leave touching bodies at exactly the
/// sum of their radii, give or take float rounding. Without this slack, a chaser resting against the
/// player would flicker between "touching" and "one hair away" from one step to the next.
inline constexpr float kContactSlack = 1.0f;

/// The fraction of an overlap that body A takes when it collides with body B: `weightB / (weightA + weightB)`.
/// A heavier body moves less. Only the ratio between the two weights matters.
[[nodiscard]] float PushShare(float weightA, float weightB);

/// If the circles of `a` and `b` overlap, pushes them apart along the line between their centres until
/// they just touch, and removes the speed at which they were closing in on each other.
///
/// It works like a perfectly inelastic collision between two weights: after it, both bodies move at the
/// same speed along that line, so the heavier one shoves the lighter one instead of passing through it.
/// Speed across the line (sliding past each other) is not touched.
///
/// @param shareA The fraction of the overlap `a` takes, as `PushShare` gives it. `b` takes the rest.
/// @return Whether the two bodies overlapped.
bool SeparateCircles(Motion& a, float radiusA, Motion& b, float radiusB, float shareA);

} // namespace horde
