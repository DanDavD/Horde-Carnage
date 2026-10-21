#include "core/Movement.h"

namespace horde {

void StepMovement(Motion& motion, Vec2 input, const MovementStats& stats, float dt) {
    const Vec2 wish = ClampLength(input, 1.0f);
    const Vec2 targetVelocity = wish * stats.maxSpeed;

    // Releasing the stick and pushing against the current velocity are both braking, and braking uses
    // its own, stronger rate. Without the second case, turning back the way you came is slower than
    // stopping, and a dodge back feels sluggish.
    const bool hasInput = LengthSquared(wish) > 0.0f;
    const bool braking = !hasInput || Dot(motion.velocity, wish) < 0.0f;
    const float rate = braking ? stats.deceleration : stats.acceleration;
    motion.velocity = MoveTowards(motion.velocity, targetVelocity, rate * dt);

    motion.previousPosition = motion.position;
    motion.position += motion.velocity * dt;
}

} // namespace horde
