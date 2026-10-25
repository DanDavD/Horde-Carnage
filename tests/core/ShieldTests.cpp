#include <doctest/doctest.h>

#include "core/Shield.h"
#include "core/Weapon.h"

using horde::Shield;
using horde::ShieldHit;
using horde::ShieldKind;
using horde::WeaponStats;

namespace {

constexpr float kDt = 1.0f / 60.0f;

Shield Barrier() {
    Shield shield;
    shield.kind = ShieldKind::Barrier;
    shield.strength = shield.stats.barrierStrength;
    return shield;
}

// Fires `gun` at the shield at its full rate for up to `seconds`, the shield stepping in between, and
// returns how long it took to break it, or a negative number if it held.
float SecondsToBreak(Shield shield, const WeaponStats& gun, float seconds) {
    const int stepsPerShot = static_cast<int>(60.0f / gun.shotsPerSecond + 0.5f);
    const int steps = static_cast<int>(seconds / kDt);
    for (int i = 0; i < steps; ++i) {
        if (i % stepsPerShot == 0 && horde::HitShield(shield, gun.damage) == ShieldHit::Broken) {
            return static_cast<float>(i) * kDt;
        }
        horde::StepShield(shield, kDt);
    }
    return -1.0f;
}

} // namespace

TEST_CASE("the shell bounces bullets under its minimum damage, and takes the rest in full") {
    Shield shell;
    const float full = shell.strength;
    CHECK(horde::HitShield(shell, horde::kSmg.damage) == ShieldHit::Bounced);
    CHECK(shell.strength == full);
    CHECK(shell.bounceFlashSeconds > 0.0f);
    CHECK(shell.hitFlashSeconds == 0.0f);

    CHECK(horde::HitShield(shell, shell.stats.shellMinimumDamage) == ShieldHit::Absorbed);
    CHECK(horde::HitShield(shell, horde::kPistol.damage) == ShieldHit::Absorbed);
    CHECK(shell.strength == doctest::Approx(full - 8.0f - 10.0f));
    CHECK(shell.hitFlashSeconds > 0.0f);
    // The shell does not grow back.
    horde::StepShield(shell, 1.0f);
    CHECK(shell.strength == doctest::Approx(full - 18.0f));
}

TEST_CASE("the barrier takes at most its maximum per bullet, and grows back to full but not past it") {
    Shield barrier = Barrier();
    const float full = barrier.strength;
    CHECK(horde::HitShield(barrier, horde::kPistol.damage) == ShieldHit::Absorbed);
    CHECK(barrier.strength == doctest::Approx(full - 6.0f));
    CHECK(horde::HitShield(barrier, 2) == ShieldHit::Absorbed);
    CHECK(barrier.strength == doctest::Approx(full - 8.0f));

    // 40 a second: 0.1 s brings back 4, and two seconds more stop at full.
    horde::StepShield(barrier, 0.1f);
    CHECK(barrier.strength == doctest::Approx(full - 4.0f));
    horde::StepShield(barrier, 2.0f);
    CHECK(barrier.strength == doctest::Approx(full));
}

TEST_CASE("a broken shield leaves the boss exposed for a while, then the other kind comes up at full strength") {
    Shield shield;
    shield.strength = 10.0f;
    CHECK(horde::HitShield(shield, horde::kPistol.damage) == ShieldHit::Broken);
    CHECK_FALSE(horde::IsShieldUp(shield));
    CHECK(shield.exposedSecondsLeft == doctest::Approx(shield.stats.exposedSeconds));

    // Exposed, every bullet goes through, whatever its damage, and the shield does not change.
    CHECK(horde::HitShield(shield, horde::kSmg.damage) == ShieldHit::Exposed);
    CHECK(horde::HitShield(shield, horde::kPistol.damage) == ShieldHit::Exposed);
    CHECK(shield.strength == 0.0f);

    for (int i = 0; i < 295; ++i) {
        horde::StepShield(shield, kDt);
    }
    CHECK_FALSE(horde::IsShieldUp(shield));
    for (int i = 0; i < 10; ++i) {
        horde::StepShield(shield, kDt);
    }
    REQUIRE(horde::IsShieldUp(shield));
    CHECK(shield.kind == ShieldKind::Barrier);
    CHECK(shield.strength == doctest::Approx(shield.stats.barrierStrength));

    // And after the barrier, the shell again.
    horde::BreakShield(shield);
    horde::StepShield(shield, shield.stats.exposedSeconds + kDt);
    CHECK(shield.kind == ShieldKind::Shell);
    CHECK(shield.strength == doctest::Approx(shield.stats.shellStrength));
}

TEST_CASE("only the pistol breaks the shell, and only the SMG breaks the barrier") {
    // Steady fire at each gun's full rate.
    const float pistolOnShell = SecondsToBreak(Shield{}, horde::kPistol, 30.0f);
    CHECK(pistolOnShell == doctest::Approx(2.33f).epsilon(0.05)); // 15 bullets, 0.167 s apart
    CHECK(SecondsToBreak(Shield{}, horde::kSmg, 30.0f) < 0.0f);

    const float smgOnBarrier = SecondsToBreak(Barrier(), horde::kSmg, 30.0f);
    CHECK(smgOnBarrier > 4.0f);
    CHECK(smgOnBarrier < 6.0f);
    CHECK(SecondsToBreak(Barrier(), horde::kPistol, 30.0f) < 0.0f);
}
