#include "core/Facing.h"

namespace horde {

void UpdateFacing(Vec2& facing, Vec2 input) {
    // Facing follows the input, not the velocity: the velocity still points the old way for a few
    // steps after a turn, and a shot must go where the player is pushing now.
    if (LengthSquared(input) > 0.0f) {
        facing = Normalized(input);
    }
}

void AimFacing(Vec2& facing, Vec2 from, Vec2 target) {
    const Vec2 toTarget = target - from;
    // A cursor right on top of the player would give a direction made of rounding noise, spinning the
    // gun from frame to frame. Keeping the last direction is calmer.
    if (LengthSquared(toTarget) > 1.0f) {
        facing = Normalized(toTarget);
    }
}

} // namespace horde
