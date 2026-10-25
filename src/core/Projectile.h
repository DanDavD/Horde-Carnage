#pragma once

#include "core/DamageSource.h"
#include "core/Movement.h"
#include "core/Vec2.h"
#include "core/Weapon.h"

namespace horde {

/// A projectile in flight: a bullet from the player's gun, or a strand of the boss's hair. It flies in a
/// straight line at constant speed until its range runs out, it hits something, or it leaves the arena.
/// Which side fired it is decided by the pool it lives in, not by a field.
struct Projectile {
    Motion motion; ///< Velocity never changes. `previousPosition` is kept for drawing and for the hit test.
    float radius = 4.0f;
    float secondsLeft = 0.0f; ///< Time until it has flown its range.
    int damage = 0;
    float knockback = 0.0f; ///< Speed added to what it hits, along its flight.
    /// What a hit on the player counts as, for the death screen. Only the boss's strands hit the player, so
    /// bullets leave it as it is.
    DamageSource source = DamageSource::HairRing;
};

/// A projectile leaving `origin` in `direction` (length 1) at `speed`, that flies `range` units at most.
/// Size, damage and knockback keep their defaults for the caller to set.
[[nodiscard]] Projectile LaunchProjectile(Vec2 origin, Vec2 direction, float speed, float range);

/// A bullet leaving `muzzle` in `direction` (length 1), with the damage, speed, range and knockback of `weapon`.
[[nodiscard]] Projectile MakeBullet(Vec2 muzzle, Vec2 direction, const WeaponStats& weapon);

/// Moves a projectile one fixed step along its velocity and counts its lifetime down.
///
/// @return Whether it is still flying. False once it has flown its range.
bool StepProjectile(Projectile& projectile, float dt);

} // namespace horde
