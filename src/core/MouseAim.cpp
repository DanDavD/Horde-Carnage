#include "core/MouseAim.h"

#include <algorithm>

namespace horde {

Vec2 MoveMouseAim(Vec2 aim, Vec2 screenDelta, float screenScale, float sensitivity, Vec2 viewSize) {
    const float toGame = screenScale > 0.0f ? sensitivity / screenScale : 0.0f;
    const Vec2 moved = aim + screenDelta * toGame;
    return Vec2{std::clamp(moved.x, 0.0f, viewSize.x), std::clamp(moved.y, 0.0f, viewSize.y)};
}

} // namespace horde
