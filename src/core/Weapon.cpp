#include "core/Weapon.h"

#include <algorithm>

namespace horde {

namespace {

// Float rounding can leave a cooldown a hair above 0 on the step it should end. Without this tolerance a
// 10 shots/s gun would wait one extra step after every shot and fire about 8.6 times a second.
constexpr float kReadyTolerance = 1e-4f;

} // namespace

void TickWeapon(Weapon& weapon, float dt) {
    weapon.cooldownSeconds = std::max(weapon.cooldownSeconds - dt, 0.0f);
    weapon.bufferedPressSeconds = std::max(weapon.bufferedPressSeconds - dt, 0.0f);
    weapon.muzzleFlashSeconds = std::max(weapon.muzzleFlashSeconds - dt, 0.0f);
}

bool PullTrigger(Weapon& weapon, bool held, bool pressed) {
    if (pressed) {
        weapon.bufferedPressSeconds = kTriggerBufferSeconds;
    }

    const bool hasPress = weapon.bufferedPressSeconds > 0.0f;
    const bool wantsToFire = weapon.stats.mode == FireMode::Auto ? (held || hasPress) : hasPress;
    if (!wantsToFire || weapon.cooldownSeconds > kReadyTolerance) {
        return false;
    }

    weapon.cooldownSeconds = 1.0f / weapon.stats.shotsPerSecond;
    weapon.bufferedPressSeconds = 0.0f;
    weapon.muzzleFlashSeconds = kMuzzleFlashSeconds;
    return true;
}

} // namespace horde
