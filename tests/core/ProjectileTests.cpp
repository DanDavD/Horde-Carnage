#include <doctest/doctest.h>

#include "core/Projectile.h"

using horde::Projectile;
using horde::Vec2;

namespace {

constexpr float kDt = 1.0f / 60.0f;

} // namespace

TEST_CASE("a bullet leaves the muzzle with the weapon's speed, damage and knockback") {
    const Projectile bullet = horde::MakeBullet(Vec2{10.0f, 20.0f}, Vec2{0.0f, -1.0f}, horde::kPistol);
    CHECK(bullet.motion.position == Vec2{10.0f, 20.0f});
    CHECK(bullet.motion.previousPosition == bullet.motion.position);
    CHECK(bullet.motion.velocity == Vec2{0.0f, -horde::kPistol.bulletSpeed});
    CHECK(bullet.damage == horde::kPistol.damage);
    CHECK(bullet.knockback == doctest::Approx(horde::kPistol.knockback));
    CHECK(bullet.secondsLeft == doctest::Approx(horde::kPistol.range / horde::kPistol.bulletSpeed));
}

TEST_CASE("a launched projectile leaves its origin at its speed, with a lifetime of its range") {
    const Projectile strand = horde::LaunchProjectile(Vec2{5.0f, 5.0f}, Vec2{0.0f, 1.0f}, 220.0f, 880.0f);
    CHECK(strand.motion.position == Vec2{5.0f, 5.0f});
    CHECK(strand.motion.previousPosition == strand.motion.position);
    CHECK(strand.motion.velocity == Vec2{0.0f, 220.0f});
    CHECK(strand.secondsLeft == doctest::Approx(4.0f));
}

TEST_CASE("a step moves a bullet in a straight line and remembers where it was") {

    Projectile bullet = horde::MakeBullet(Vec2{0.0f, 0.0f}, Vec2{1.0f, 0.0f}, horde::kSmg);
    CHECK(horde::StepProjectile(bullet, kDt));
    CHECK(bullet.motion.previousPosition == Vec2{0.0f, 0.0f});
    CHECK(bullet.motion.position.x == doctest::Approx(horde::kSmg.bulletSpeed * kDt));
    CHECK(bullet.motion.position.y == 0.0f);
}

TEST_CASE("a bullet flies its range and then stops flying") {
    Projectile bullet = horde::MakeBullet(Vec2{0.0f, 0.0f}, Vec2{1.0f, 0.0f}, horde::kPistol);
    int steps = 0;
    while (horde::StepProjectile(bullet, kDt)) {
        ++steps;
        REQUIRE(steps < 1000);
    }
    // The last step can overshoot the range by at most one step of flight.
    const float stepLength = horde::kPistol.bulletSpeed * kDt;
    CHECK(bullet.motion.position.x >= horde::kPistol.range - stepLength);
    CHECK(bullet.motion.position.x <= horde::kPistol.range + stepLength);
}
