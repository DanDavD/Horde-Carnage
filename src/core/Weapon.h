#pragma once

#include <string_view>

#include "core/Vec2.h"

namespace horde {

/// How a gun responds to the fire button.
enum class FireMode {
    SemiAuto, ///< One shot per press. Holding the button does nothing more.
    Auto,     ///< Keeps firing while the button is held.
};

/// What a gun is, as opposed to its state. Day 9 moves these numbers to JSON.
struct WeaponStats {
    std::string_view name;
    FireMode mode = FireMode::SemiAuto;
    /// Highest fire rate. A semi-automatic gun cannot go faster however fast the player clicks.
    float shotsPerSecond = 6.0f;
    int damage = 10;             ///< Hit points one bullet takes.
    float bulletSpeed = 2090.0f; ///< Units per second.
    float range = 750.0f;        ///< How far a bullet flies before it is gone.
    float knockback = 300.0f;    ///< Speed a bullet adds to what it hits.
    float barrelLength = 18.0f;  ///< How far the muzzle sticks out of the body. Bullets start there.
};

/// The starting guns. The pistol rewards clicking with harder, longer shots; the SMG fires a steadier
/// stream of weaker bullets just by holding. At 60 Hz both fire rates are a whole number of steps
/// (10 and 6), so they are exact. Against a 30 hit point hairball the pistol needs 3 bullets, the SMG 5.
inline constexpr WeaponStats kPistol{.name = "Pistol",
                                     .mode = FireMode::SemiAuto,
                                     .shotsPerSecond = 6.0f,
                                     .damage = 10,
                                     .bulletSpeed = 2090.0f,
                                     .range = 750.0f,
                                     .knockback = 300.0f,
                                     .barrelLength = 18.0f};
inline constexpr WeaponStats kSmg{.name = "SMG",
                                  .mode = FireMode::Auto,
                                  .shotsPerSecond = 10.0f,
                                  .damage = 6,
                                  .bulletSpeed = 1870.0f,
                                  .range = 600.0f,
                                  .knockback = 120.0f,
                                  .barrelLength = 30.0f};

/// Where the muzzle of `weapon` is, held by a body of `bodyRadius` at `center` that faces `facing`.
/// The simulation fires bullets from here and the renderer draws the barrel up to here, so both agree.
[[nodiscard]] constexpr Vec2 MuzzlePoint(Vec2 center, Vec2 facing, float bodyRadius, const WeaponStats& weapon) {
    return center + facing * (bodyRadius + weapon.barrelLength);
}

/// A press that arrives a little before the gun is ready is kept this long and fires as soon as it is.
/// Without it, a quick double click on the pistol would lose the second shot without telling the player.
inline constexpr float kTriggerBufferSeconds = 0.15f;

/// How long the muzzle flash shows after a shot.
inline constexpr float kMuzzleFlashSeconds = 0.05f;

/// One gun the player carries, with its timers.
struct Weapon {
    WeaponStats stats;
    float cooldownSeconds = 0.0f;      ///< Time until it can fire again. At 0 it is ready.
    float bufferedPressSeconds = 0.0f; ///< Time left on an early press that waits for the cooldown.
    float muzzleFlashSeconds = 0.0f;   ///< Time left on the flash of the last shot.
};

/// Advances the weapon's timers by `dt` seconds.
void TickWeapon(Weapon& weapon, float dt);

/// Decides whether the weapon fires in this step and, if it does, starts its cooldown and its flash.
///
/// A semi-automatic weapon fires on a press (or a press buffered a moment ago). An automatic weapon
/// also fires while the button is held. Both wait for the cooldown between shots.
///
/// @param canFire False while something else has the player's hands (a dash). The press is still
///        buffered, so a click during a dash fires as soon as the dash ends.
/// @return Whether a shot was fired.
bool PullTrigger(Weapon& weapon, bool held, bool pressed, bool canFire = true);

} // namespace horde
