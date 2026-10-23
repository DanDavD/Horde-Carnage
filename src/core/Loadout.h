#pragma once

#include <array>

#include "core/Weapon.h"

namespace horde {

inline constexpr int kWeaponSlots = 2;

/// The guns the player carries and which one is in hand. Slot 0 is the pistol, slot 1 the SMG.
struct Loadout {
    std::array<Weapon, kWeaponSlots> slots{Weapon{.stats = kPistol}, Weapon{.stats = kSmg}};
    int equipped = 0;
};

[[nodiscard]] Weapon& Equipped(Loadout& loadout);
[[nodiscard]] const Weapon& Equipped(const Loadout& loadout);

/// Puts the gun in `slot` in hand. A slot that does not exist is ignored.
void SelectSlot(Loadout& loadout, int slot);

/// Puts the next gun in hand, wrapping around after the last one.
void CycleWeapon(Loadout& loadout);

/// Advances the timers of every gun, not just the one in hand, so a gun cools down while it is put away.
void TickLoadout(Loadout& loadout, float dt);

} // namespace horde
