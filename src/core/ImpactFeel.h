#pragma once

#include "core/WorldEvent.h"

namespace horde {

/// A hit from this much damage up is a heavy one: the Charge (30) and the Leap's impact (35). It slows and
/// shakes harder than a strand or a touch.
inline constexpr int kHeavyHitDamage = 30;

/// How hard an event hits the player's senses: how long the fight slows down and how much the screen shakes.
struct ImpactFeel {
    float slowSeconds = 0.0f; ///< How long the hit stop lasts, in real seconds. See `HitStop`.
    float trauma = 0.0f;
};

/// The slow-down and the shake for one event, from the table in `ImpactFeel.cpp`. Events that happen many
/// times a second (shots, bullets on the boss) neither slow nor shake: they would blur into a constant
/// tremor and hide the hits that matter.
[[nodiscard]] ImpactFeel FeelOf(const WorldEvent& event);

} // namespace horde
