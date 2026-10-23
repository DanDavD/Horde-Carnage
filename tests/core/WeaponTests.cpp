// doctest prints the operands of a failed check. For std::string_view that needs a complete std::ostream.
#include <ostream>

#include <doctest/doctest.h>

#include "core/Weapon.h"

using horde::FireMode;
using horde::Vec2;
using horde::Weapon;
using horde::WeaponStats;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// One fixed step of a gun: time passes, then the trigger is read, the same order as StepWorld.
bool Step(Weapon& weapon, bool held, bool pressed) {
    horde::TickWeapon(weapon, kDt);
    return horde::PullTrigger(weapon, held, pressed);
}

// How many shots a gun fires over `steps` steps with the button held, pressed only on the first step.
int HoldFor(Weapon weapon, int steps) {
    int shots = 0;
    for (int i = 0; i < steps; ++i) {
        shots += Step(weapon, true, i == 0) ? 1 : 0;
    }
    return shots;
}

} // namespace

TEST_CASE("the starting guns are a semi-automatic pistol and an automatic SMG") {
    CHECK(horde::kPistol.mode == FireMode::SemiAuto);
    CHECK(horde::kSmg.mode == FireMode::Auto);
    CHECK(horde::kPistol.name == "Pistol");
    CHECK(horde::kSmg.name == "SMG");
}

TEST_CASE("a press fires a ready gun at once and starts the cooldown and the flash") {
    Weapon weapon{.stats = horde::kPistol};
    CHECK(Step(weapon, true, true));
    CHECK(weapon.cooldownSeconds == doctest::Approx(1.0f / 6.0f));
    CHECK(weapon.muzzleFlashSeconds == doctest::Approx(horde::kMuzzleFlashSeconds));
}

TEST_CASE("holding a semi-automatic trigger fires only once") {
    CHECK(HoldFor(Weapon{.stats = horde::kPistol}, 60) == 1);
}

TEST_CASE("holding an automatic trigger fires at exactly its rate") {
    CHECK(HoldFor(Weapon{.stats = horde::kSmg}, 60) == 10);
    CHECK(HoldFor(Weapon{.stats = horde::kSmg}, 120) == 20);
}

TEST_CASE("clicking faster than the fire rate still fires no faster than it") {
    Weapon weapon{.stats = horde::kPistol};
    int shots = 0;
    for (int i = 0; i < 60; ++i) {
        shots += Step(weapon, false, true) ? 1 : 0;
    }
    CHECK(shots == 6);
}

TEST_CASE("a press just before the gun is ready fires as soon as it is") {
    Weapon weapon{.stats = horde::kPistol}; // Ready again 10 steps after a shot.
    REQUIRE(Step(weapon, false, true));
    for (int i = 0; i < 6; ++i) {
        CHECK_FALSE(Step(weapon, false, false));
    }

    // 7 steps after the shot, 3 steps (0.05 s) early: inside the 0.15 s buffer.
    CHECK_FALSE(Step(weapon, false, true));
    CHECK_FALSE(Step(weapon, false, false));
    CHECK_FALSE(Step(weapon, false, false));
    CHECK(Step(weapon, false, false));
}

TEST_CASE("a press that comes much too early is dropped, not fired later") {
    // A slow test gun, ready again 0.5 s after a shot.
    Weapon weapon{.stats = WeaponStats{.name = "Slow", .mode = FireMode::SemiAuto, .shotsPerSecond = 2.0f}};
    REQUIRE(Step(weapon, false, true));

    // Pressed one step after the shot, about 0.48 s before the gun is ready: the 0.15 s buffer runs out first.
    CHECK_FALSE(Step(weapon, false, true));
    int shots = 0;
    for (int i = 0; i < 30; ++i) {
        shots += Step(weapon, false, false) ? 1 : 0;
    }
    CHECK(shots == 0);
}

TEST_CASE("the timers never count below zero") {
    Weapon weapon{.stats = horde::kSmg};
    REQUIRE(Step(weapon, true, false));
    horde::TickWeapon(weapon, 5.0f);
    CHECK(weapon.cooldownSeconds == 0.0f);
    CHECK(weapon.bufferedPressSeconds == 0.0f);
    CHECK(weapon.muzzleFlashSeconds == 0.0f);
}

TEST_CASE("a gun with no press and no held button does not fire") {
    Weapon weapon{.stats = WeaponStats{.name = "Test", .mode = FireMode::Auto, .shotsPerSecond = 5.0f}};
    CHECK_FALSE(Step(weapon, false, false));
}

TEST_CASE("the muzzle is at the end of the barrel, outside the body") {
    const Vec2 muzzle = horde::MuzzlePoint(Vec2{100.0f, 100.0f}, Vec2{0.0f, 1.0f}, 20.0f, horde::kSmg);
    CHECK(muzzle == Vec2{100.0f, 100.0f + 20.0f + horde::kSmg.barrelLength});
}

TEST_CASE("the pistol hits harder per bullet and the SMG fires faster") {
    CHECK(horde::kPistol.damage > horde::kSmg.damage);
    CHECK(horde::kSmg.shotsPerSecond > horde::kPistol.shotsPerSecond);
}
