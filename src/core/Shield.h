#pragma once

namespace horde {

/// The two shields the boss alternates between. Each one only gives way to one of the two guns, and the
/// rule is a number, not a check of which gun fired, so a new gun falls on one side or the other by its
/// damage per bullet.
enum class ShieldKind {
    Shell,   ///< Blue and hard: bullets that hit for less than `ShieldStats::shellMinimumDamage` bounce off.
    Barrier, ///< Orange and soft: it takes at most `barrierMaximumDamage` per bullet and grows back.
};

/// The numbers of both shields.
///
/// With the starting guns (pistol 10 damage at 6 shots/s, SMG 6 damage at 10 shots/s, both 60 a second):
/// the SMG cannot hurt the Shell at all, and the pistol breaks it in 2.5 s of steady fire. Against the
/// Barrier the pistol's hits count as 6, 36 a second, less than the 40 it grows back; the SMG's 60 a
/// second beats it by 20 and breaks it in 5 s.
struct ShieldStats {
    float shellStrength = 150.0f; ///< 15 pistol bullets.
    int shellMinimumDamage = 8;   ///< Hits below this bounce off and take nothing.
    float barrierStrength = 100.0f;
    int barrierMaximumDamage = 6;       ///< No single hit takes more than this.
    float barrierRegeneration = 40.0f;  ///< Strength per second it grows back while it is up.
    float exposedSeconds = 5.0f;        ///< How long the boss is open to every bullet once a shield breaks.
    float flashSeconds = 0.1f;          ///< The flash of a hit that counted, or of a bounce.
};

/// The boss's shield: up, with some strength left, or broken and leaving the boss exposed for a while.
/// When the exposed time runs out, the other kind comes up at full strength.
struct Shield {
    ShieldKind kind = ShieldKind::Shell; ///< The fight opens with the Shell, which the pistol in hand breaks.
    float strength = ShieldStats{}.shellStrength;
    float exposedSecondsLeft = 0.0f; ///< Above 0 while the shield is down.
    float hitFlashSeconds = 0.0f;    ///< A bullet that counted.
    float bounceFlashSeconds = 0.0f; ///< A bullet that bounced off.
    ShieldStats stats;
};

/// What one bullet did to the shield.
enum class ShieldHit {
    Bounced,  ///< The Shell shrugged it off.
    Absorbed, ///< It took strength off the shield, which held.
    Broken,   ///< It took the last of the strength: the boss is exposed from now on.
    Exposed,  ///< There was no shield: the bullet hits the boss itself.
};

[[nodiscard]] constexpr bool IsShieldUp(const Shield& shield) {
    return shield.exposedSecondsLeft <= 0.0f;
}

/// The full strength of `kind`.
[[nodiscard]] float MaxStrength(const ShieldStats& stats, ShieldKind kind);

/// Applies one bullet of `damage` to the shield, by the rules of its kind.
ShieldHit HitShield(Shield& shield, int damage);

/// Advances the shield by `dt` seconds: the Barrier grows back while it is up, the flashes fade, and once
/// the exposed time is over the other kind comes up at full strength.
void StepShield(Shield& shield, float dt);

/// Takes the shield down at once, for the Debug shortcut that skips to the exposed window.
void BreakShield(Shield& shield);

} // namespace horde
