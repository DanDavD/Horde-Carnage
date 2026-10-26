#include "core/ScreenFit.h"

#include <algorithm>

namespace horde {

ScreenFit FitScreen(Vec2 gameSize, Vec2 screenSize) {
    if (gameSize.x <= 0.0f || gameSize.y <= 0.0f || screenSize.x <= 0.0f || screenSize.y <= 0.0f) {
        return ScreenFit{};
    }
    const float scale = std::min(screenSize.x / gameSize.x, screenSize.y / gameSize.y);
    return ScreenFit{.scale = scale, .offset = (screenSize - gameSize * scale) * 0.5f};
}

Vec2 ScreenToGame(const ScreenFit& fit, Vec2 screenPoint) {
    return (screenPoint - fit.offset) * (1.0f / fit.scale);
}

Vec2 GameToScreen(const ScreenFit& fit, Vec2 gamePoint) {
    return fit.offset + gamePoint * fit.scale;
}

} // namespace horde
