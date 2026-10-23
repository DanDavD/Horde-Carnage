#pragma once

#include <raylib.h>

#include "core/Vec2.h"

namespace horde {

/// horde_core never sees raylib types. The app converts here, right before drawing.
[[nodiscard]] inline Vector2 ToRaylib(Vec2 v) {
    return Vector2{v.x, v.y};
}

/// The other way, for input that raylib reports in its own types (the mouse).
[[nodiscard]] inline Vec2 FromRaylib(Vector2 v) {
    return Vec2{v.x, v.y};
}

} // namespace horde
