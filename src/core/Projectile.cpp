#include "core/Projectile.h"

namespace horde {

Projectile MakeBullet(Vec2 muzzle, Vec2 direction, const WeaponStats& weapon) {
    Projectile bullet{.motion = MotionAt(muzzle), .damage = weapon.damage, .knockback = weapon.knockback};
    bullet.motion.velocity = direction * weapon.bulletSpeed;
    // Range as a lifetime: at constant speed the two are the same, and a timer is one subtraction a step.
    bullet.secondsLeft = weapon.range / weapon.bulletSpeed;
    return bullet;
}

bool StepProjectile(Projectile& projectile, float dt) {
    projectile.motion.previousPosition = projectile.motion.position;
    projectile.motion.position += projectile.motion.velocity * dt;
    projectile.secondsLeft -= dt;
    return projectile.secondsLeft > 0.0f;
}

} // namespace horde
