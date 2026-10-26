#include "app/render/View.h"

#include <algorithm>

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"

namespace horde {

ScreenFit CurrentScreenFit() {
    return FitScreen(kViewSize, Vec2{static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())});
}

Vec2 AimInGame(const ScreenFit& fit) {
    const Vec2 aim = ScreenToGame(fit, ReadAimPoint());
    return Vec2{std::clamp(aim.x, 0.0f, kViewSize.x), std::clamp(aim.y, 0.0f, kViewSize.y)};
}

Camera2D ScreenCamera(const ScreenFit& fit) {
    return Camera2D{.offset = ToRaylib(fit.offset), .target = {0.0f, 0.0f}, .rotation = 0.0f, .zoom = fit.scale};
}

Camera2D ToRaylibCamera(const FollowCamera& camera, float alpha) {
    // The camera is interpolated like the player. If only the player were, the two would drift apart
    // by a fraction of a step every frame and the player would shimmer against the floor.
    const Vec2 center = Lerp(camera.previousCenter, camera.center, alpha);
    return Camera2D{.offset = {kScreenWidth * 0.5f, kScreenHeight * 0.5f},
                    .target = ToRaylib(center),
                    .rotation = 0.0f,
                    .zoom = 1.0f};
}

Camera2D WorldCamera(const FollowCamera& camera, float alpha, const ScreenFit& fit) {
    Camera2D world = ToRaylibCamera(camera, alpha);
    world.offset = ToRaylib(GameToScreen(fit, kViewSize * 0.5f));
    world.zoom = fit.scale;
    return world;
}

} // namespace horde
