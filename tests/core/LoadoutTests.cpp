// doctest prints the operands of a failed check. For std::string_view that needs a complete std::ostream.
#include <ostream>

#include <doctest/doctest.h>

#include "core/Loadout.h"

using horde::Loadout;

TEST_CASE("the player starts with the pistol in hand and the SMG in the second slot") {
    const Loadout loadout;
    CHECK(loadout.equipped == 0);
    CHECK(horde::Equipped(loadout).stats.name == "Pistol");
    CHECK(loadout.slots[1].stats.name == "SMG");
}

TEST_CASE("selecting a slot puts that gun in hand") {
    Loadout loadout;
    horde::SelectSlot(loadout, 1);
    CHECK(horde::Equipped(loadout).stats.name == "SMG");
    horde::SelectSlot(loadout, 0);
    CHECK(horde::Equipped(loadout).stats.name == "Pistol");
}

TEST_CASE("a slot that does not exist is ignored") {
    Loadout loadout;
    horde::SelectSlot(loadout, 5);
    horde::SelectSlot(loadout, -1);
    CHECK(loadout.equipped == 0);
}

TEST_CASE("cycling goes to the next gun and wraps around") {
    Loadout loadout;
    horde::CycleWeapon(loadout);
    CHECK(loadout.equipped == 1);
    horde::CycleWeapon(loadout);
    CHECK(loadout.equipped == 0);
}

TEST_CASE("a gun keeps cooling down while it is put away") {
    Loadout loadout;
    loadout.slots[0].cooldownSeconds = 0.5f;
    horde::SelectSlot(loadout, 1);
    horde::TickLoadout(loadout, 0.25f);
    CHECK(loadout.slots[0].cooldownSeconds == doctest::Approx(0.25f));
}

TEST_CASE("switching away drops a press buffered on the old gun") {
    Loadout loadout;
    loadout.slots[0].bufferedPressSeconds = 0.1f;
    horde::SelectSlot(loadout, 1);
    CHECK(loadout.slots[0].bufferedPressSeconds == 0.0f);
}
