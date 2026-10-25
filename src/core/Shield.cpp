#include "core/Shield.h"

#include <algorithm>

namespace horde {

float MaxStrength(const ShieldStats& stats, ShieldKind kind) {
    return kind == ShieldKind::Shell ? stats.shellStrength : stats.barrierStrength;
}

ShieldHit HitShield(Shield& shield, int damage) {
    if (!IsShieldUp(shield)) {
        return ShieldHit::Exposed;
    }
    const ShieldStats& stats = shield.stats;
    float taken = static_cast<float>(damage);
    if (shield.kind == ShieldKind::Shell) {
        if (damage < stats.shellMinimumDamage) {
            shield.bounceFlashSeconds = stats.flashSeconds;
            return ShieldHit::Bounced;
        }
    } else {
        taken = std::min(taken, static_cast<float>(stats.barrierMaximumDamage));
    }
    shield.hitFlashSeconds = stats.flashSeconds;
    shield.strength -= taken;
    if (shield.strength > 0.0f) {
        return ShieldHit::Absorbed;
    }
    BreakShield(shield);
    return ShieldHit::Broken;
}

void StepShield(Shield& shield, float dt) {
    const ShieldStats& stats = shield.stats;
    shield.hitFlashSeconds = std::max(shield.hitFlashSeconds - dt, 0.0f);
    shield.bounceFlashSeconds = std::max(shield.bounceFlashSeconds - dt, 0.0f);

    if (IsShieldUp(shield)) {
        if (shield.kind == ShieldKind::Barrier) {
            shield.strength = std::min(shield.strength + stats.barrierRegeneration * dt, stats.barrierStrength);
        }
        return;
    }
    shield.exposedSecondsLeft -= dt;
    if (shield.exposedSecondsLeft <= 0.0f) {
        // The other kind, so the player has to switch guns for every shield.
        shield.exposedSecondsLeft = 0.0f;
        shield.kind = shield.kind == ShieldKind::Shell ? ShieldKind::Barrier : ShieldKind::Shell;
        shield.strength = MaxStrength(stats, shield.kind);
    }
}

void BreakShield(Shield& shield) {
    shield.strength = 0.0f;
    shield.exposedSecondsLeft = shield.stats.exposedSeconds;
}

} // namespace horde
