#pragma once

#include <algorithm>
#include <cassert>

#include "core/Vec2.h"

namespace horde {

/// Ignores small stick values (worn sticks never rest exactly at zero) and rescales the rest,
/// so the output length goes from 0 at the edge of the deadzone to 1 at full tilt.
///
/// The deadzone is radial: it looks at the length of the stick vector, not at each axis on its own,
/// so diagonals are not pulled toward the four main directions.
[[nodiscard]] inline Vec2 ApplyRadialDeadzone(Vec2 stick, float deadzone) {
    assert(deadzone >= 0.0f && deadzone < 1.0f && "the deadzone must be in [0, 1)");
    const float length = Length(stick);
    if (length <= deadzone) {
        return {};
    }
    // Some gamepads report corners longer than 1, so the rescaled length is capped too.
    const float scaledLength = std::min((length - deadzone) / (1.0f - deadzone), 1.0f);
    return stick * (scaledLength / length);
}

} // namespace horde
