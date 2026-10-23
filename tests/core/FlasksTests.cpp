#include <doctest/doctest.h>

#include "core/Flasks.h"
#include "core/Health.h"

using horde::Flasks;
using horde::Health;

namespace {

// Steps of 0.25 s, so the 1 s drink lasts exactly four steps.
constexpr float kDt = 0.25f;

} // namespace

TEST_CASE("a new set of flasks is full and not drinking") {
    const Flasks flasks;
    CHECK(flasks.charges == flasks.stats.maxCharges);
    CHECK_FALSE(horde::IsDrinking(flasks));
}

TEST_CASE("starting a drink spends a charge at once") {
    Flasks flasks;
    CHECK(horde::TryStartDrinking(flasks));
    CHECK(horde::IsDrinking(flasks));
    CHECK(flasks.charges == flasks.stats.maxCharges - 1);
}

TEST_CASE("a drink cannot start while another is in progress or with no charges left") {
    Flasks flasks;
    REQUIRE(horde::TryStartDrinking(flasks));
    CHECK_FALSE(horde::TryStartDrinking(flasks));

    Flasks empty{.charges = 0};
    CHECK_FALSE(horde::TryStartDrinking(empty));
}

TEST_CASE("the heal arrives once, when the drink finishes") {
    Flasks flasks;
    REQUIRE(horde::TryStartDrinking(flasks));
    CHECK(horde::StepFlasks(flasks, kDt) == 0);
    CHECK(horde::StepFlasks(flasks, kDt) == 0);
    CHECK(horde::StepFlasks(flasks, kDt) == 0);
    CHECK(horde::StepFlasks(flasks, kDt) == flasks.stats.heal);
    CHECK_FALSE(horde::IsDrinking(flasks));
    CHECK(horde::StepFlasks(flasks, kDt) == 0);
}

TEST_CASE("an interrupted drink heals nothing and the charge stays spent") {
    Flasks flasks;
    REQUIRE(horde::TryStartDrinking(flasks));
    CHECK(horde::StepFlasks(flasks, kDt) == 0);
    horde::InterruptDrinking(flasks);
    CHECK_FALSE(horde::IsDrinking(flasks));
    CHECK(horde::StepFlasks(flasks, kDt) == 0);
    CHECK(flasks.charges == flasks.stats.maxCharges - 1);
}

TEST_CASE("healing restores health up to the maximum and never above") {
    Health health{.current = 50, .max = 100};
    CHECK(horde::Heal(health, 40));
    CHECK(health.current == 90);
    CHECK(horde::Heal(health, 40));
    CHECK(health.current == 100);
    CHECK_FALSE(horde::Heal(health, 40));
}

TEST_CASE("the dead cannot be healed") {
    Health health{.current = 0, .max = 100};
    CHECK_FALSE(horde::Heal(health, 40));
    CHECK(health.current == 0);
}
