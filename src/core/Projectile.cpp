#include "core/Projectile.h"

namespace horde {

Projectile LaunchProjectile(Vec2 origin, Vec2 direction, float speed, float range) {
    Projectile projectile{.motion = MotionAt(origin)};
    projectile.motion.velocity = direction * speed;
    // Range as a lifetime: at constant speed the two are the same, and a timer is one subtraction a step.
    projectile.secondsLeft = range / speed;
    return projectile;
}

Projectile MakeBullet(Vec2 muzzle, Vec2 direction, const WeaponStats& weapon) {
    Projectile bullet = LaunchProjectile(muzzle, direction, weapon.bulletSpeed, weapon.range);
    bullet.damage = weapon.damage;
    bullet.knockback = weapon.knockback;
    return bullet;
}

bool StepProjectile(Projectile& projectile, float dt) {
    projectile.motion.previousPosition = projectile.motion.position;
    projectile.motion.position += projectile.motion.velocity * dt;
    projectile.secondsLeft -= dt;
    return projectile.secondsLeft > 0.0f;
}

} // namespace horde
