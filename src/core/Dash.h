#pragma once

#include "core/Vec2.h"

namespace horde {

/// How the dash moves. It is a burst too fast to follow with the eye, so it reads as a short teleport, but
/// the body really travels the distance: walls stop it, and it cannot end up inside one.
struct DashStats {
    float distance = 180.0f; ///< Units covered by one dash, about four body widths.
    float duration = 0.12f;  ///< Seconds the burst lasts.
    /// Seconds the body cannot be hurt, from the start. A little longer than the burst, so landing right
    /// at the edge of an attack is still forgiven.
    float invulnerability = 0.15f;
    float cooldown = 0.5f; ///< Seconds after the burst ends before the next dash.
};

/// The dash component: what the current dash is doing and when the next one is allowed.
struct Dash {
    DashStats stats;
    Vec2 direction;                   ///< Length 1 while dashing.
    Vec2 origin;                      ///< Where the current dash started, for the trail.
    float activeSeconds = 0.0f;       ///< Time left in the burst. Above 0 means dashing.
    float invulnerableSeconds = 0.0f; ///< Time left in which the body cannot be hurt.
    float cooldownSeconds = 0.0f;     ///< Time until the next dash can start.
};

[[nodiscard]] constexpr bool IsDashing(const Dash& dash) {
    return dash.activeSeconds > 0.0f;
}

[[nodiscard]] constexpr bool IsDashInvulnerable(const Dash& dash) {
    return dash.invulnerableSeconds > 0.0f;
}

/// Starts a dash from `origin` toward `direction` (any length) if the cooldown is over. A zero direction
/// has nowhere to go, so it starts nothing.
///
/// @return Whether a dash started.
bool TryStartDash(Dash& dash, Vec2 origin, Vec2 direction);

/// How far the body moves in this step of the dash, and counts the burst and the invulnerability down.
///
/// The last step of the burst moves only for the time that was left, so a dash always covers exactly
/// `distance`, whatever the length of the step. Returns a zero vector when not dashing.
[[nodiscard]] Vec2 StepDash(Dash& dash, float dt);

/// Counts the cooldown down. It runs every step, dashing or not.
void TickDashCooldown(Dash& dash, float dt);

} // namespace horde
