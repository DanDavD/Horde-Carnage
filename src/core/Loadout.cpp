#include "core/Loadout.h"

#include <cstddef>

namespace horde {

Weapon& Equipped(Loadout& loadout) {
    return loadout.slots[static_cast<std::size_t>(loadout.equipped)];
}

const Weapon& Equipped(const Loadout& loadout) {
    return loadout.slots[static_cast<std::size_t>(loadout.equipped)];
}

void SelectSlot(Loadout& loadout, int slot) {
    if (slot < 0 || slot >= kWeaponSlots || slot == loadout.equipped) {
        return;
    }
    // A press buffered on the gun being put away must not fire later, when the player switches back.
    Equipped(loadout).bufferedPressSeconds = 0.0f;
    loadout.equipped = slot;
}

void CycleWeapon(Loadout& loadout) {
    SelectSlot(loadout, (loadout.equipped + 1) % kWeaponSlots);
}

void TickLoadout(Loadout& loadout, float dt) {
    for (Weapon& weapon : loadout.slots) {
        TickWeapon(weapon, dt);
    }
}

} // namespace horde
