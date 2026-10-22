#include "core/Chase.h"

namespace horde {

Vec2 ChaseInput(Vec2 from, Vec2 target, float stopDistance) {
    const Vec2 toTarget = target - from;
    // Without a stop distance the chaser would aim at the centre of the player, overshoot it and turn
    // back every few steps, which looks like shaking. Stopping at contact also leaves the touch itself
    // to the contact damage that comes next.
    if (LengthSquared(toTarget) <= stopDistance * stopDistance) {
        return {};
    }
    return Normalized(toTarget);
}

} // namespace horde
