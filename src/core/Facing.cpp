#include "core/Facing.h"

namespace horde {

void UpdateFacing(Vec2& facing, Vec2 input) {
    // Facing follows the input, not the velocity: the velocity still points the old way for a few
    // steps after a turn, and an attack must go where the player is pushing now.
    if (LengthSquared(input) > 0.0f) {
        facing = Normalized(input);
    }
}

} // namespace horde
