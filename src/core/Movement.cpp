#include "core/Movement.h"

namespace horde {

void StepMovement(Motion& motion, Vec2 input, const MovementStats& stats, float dt) {
    const Vec2 wish = ClampLength(input, 1.0f);
    const Vec2 targetVelocity = wish * stats.maxSpeed;

    // Releasing the stick uses its own rate, so stopping can feel sharper than starting.
    const bool hasInput = LengthSquared(wish) > 0.0f;
    const float rate = hasInput ? stats.acceleration : stats.deceleration;
    motion.velocity = MoveTowards(motion.velocity, targetVelocity, rate * dt);

    motion.previousPosition = motion.position;
    motion.position += motion.velocity * dt;
}

} // namespace horde
