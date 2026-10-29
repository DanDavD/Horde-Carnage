#pragma once

#include "core/Vec2.h"

namespace horde {

/// The crosshair `aim`, in game pixels, moved by the mouse's movement `screenDelta` in screen pixels: turned
/// into game pixels by the screen's scale (`screenScale` screen pixels to a game pixel), then multiplied by
/// `sensitivity`. It never leaves the view of size `viewSize`.
///
/// While a fight is on, the game locks the system cursor and moves its own crosshair this way, so the
/// sensitivity can make it faster or slower than the cursor, and the mouse never runs into the screen's edge.
[[nodiscard]] Vec2 MoveMouseAim(Vec2 aim, Vec2 screenDelta, float screenScale, float sensitivity, Vec2 viewSize);

} // namespace horde
