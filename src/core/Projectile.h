#pragma once

#include "core/Movement.h"
#include "core/Vec2.h"
#include "core/Weapon.h"

namespace horde {

/// A bullet in flight. It flies in a straight line at constant speed until its range runs out, it hits
/// something, or it leaves the arena.
struct Projectile {
    Motion motion; ///< Velocity never changes. `previousPosition` is kept for drawing and for the hit test.
    float radius = 4.0f;
    float secondsLeft = 0.0f; ///< Time until the bullet has flown its range.
    int damage = 0;
    float knockback = 0.0f; ///< Speed added to what it hits, along its flight.
};

/// A bullet leaving `muzzle` in `direction` (length 1), with the damage, speed, range and knockback of `weapon`.
[[nodiscard]] Projectile MakeBullet(Vec2 muzzle, Vec2 direction, const WeaponStats& weapon);

/// Moves a bullet one fixed step along its velocity and counts its lifetime down.
///
/// @return Whether it is still flying. False once it has flown its range.
bool StepProjectile(Projectile& projectile, float dt);

} // namespace horde
