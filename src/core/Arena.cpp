#include "core/Arena.h"

#include <algorithm>

namespace horde {

namespace {

// One axis at a time, so touching a side wall leaves the vertical motion alone.
void ConfineAxis(float& position, float& velocity, float min, float max, float radius) {
    const float low = min + radius;
    const float high = max - radius;
    if (low > high) {
        // The arena is narrower than the mover. Centring it is the only answer that is not a wall.
        position = (min + max) * 0.5f;
        velocity = 0.0f;
        return;
    }

    // Without cancelling the velocity into the wall, it would stay at full speed while the mover
    // is pinned, and walking away would first have to brake that invisible speed: the wall would feel sticky.
    if (position < low) {
        position = low;
        velocity = std::max(velocity, 0.0f);
    } else if (position > high) {
        position = high;
        velocity = std::min(velocity, 0.0f);
    }
}

} // namespace

void ConfineToArena(Motion& motion, float radius, const Arena& arena) {
    ConfineAxis(motion.position.x, motion.velocity.x, arena.min.x, arena.max.x, radius);
    ConfineAxis(motion.position.y, motion.velocity.y, arena.min.y, arena.max.y, radius);
}

} // namespace horde
