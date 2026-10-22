#pragma once

namespace horde {

/// Hit points, plus a short window after each hit in which the body cannot be hurt again.
///
/// The window matters for contact damage: a hairball resting against the player touches it on every
/// fixed step, 60 times a second. Without the window, that would empty the health bar in a sixth of a
/// second, before the player could react.
struct Health {
    int current = 100;
    int max = 100;
    float invulnerabilityAfterHit = 0.8f; ///< Seconds a hit protects the body from the next one.
    float invulnerableSeconds = 0.0f;     ///< Time left in the current window. 0 means it can be hurt.
};

[[nodiscard]] constexpr bool IsDead(const Health& health) {
    return health.current <= 0;
}

[[nodiscard]] constexpr bool IsInvulnerable(const Health& health) {
    return health.invulnerableSeconds > 0.0f;
}

/// Takes `amount` hit points unless the body is dead or still protected by the last hit, and starts a
/// new invulnerability window. Health never goes below 0.
///
/// @return Whether the hit landed.
bool ApplyDamage(Health& health, int amount);

/// Counts the invulnerability window down by `dt` seconds.
void TickHealth(Health& health, float dt);

} // namespace horde
