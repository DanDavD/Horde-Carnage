#pragma once

#include <string_view>

namespace horde {

/// How a gun responds to the fire button.
enum class FireMode {
    SemiAuto, ///< One shot per press. Holding the button does nothing more.
    Auto,     ///< Keeps firing while the button is held.
};

/// What a gun is, as opposed to its state. Day 6 moves these numbers to JSON.
struct WeaponStats {
    std::string_view name;
    FireMode mode = FireMode::SemiAuto;
    /// Highest fire rate. A semi-automatic gun cannot go faster however fast the player clicks.
    float shotsPerSecond = 6.0f;
};

/// The starting guns. The pistol rewards clicking; the SMG fires a steadier stream just by holding.
/// At 60 Hz both rates are a whole number of steps (10 and 6), so they are exact.
inline constexpr WeaponStats kPistol{.name = "Pistol", .mode = FireMode::SemiAuto, .shotsPerSecond = 6.0f};
inline constexpr WeaponStats kSmg{.name = "SMG", .mode = FireMode::Auto, .shotsPerSecond = 10.0f};

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
/// @return Whether a shot was fired.
bool PullTrigger(Weapon& weapon, bool held, bool pressed);

} // namespace horde
