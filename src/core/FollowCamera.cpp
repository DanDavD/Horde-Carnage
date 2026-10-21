#include "core/FollowCamera.h"

#include <algorithm>
#include <cmath>

namespace horde {

namespace {

float ClampViewAxis(float center, float viewSize, float min, float max) {
    const float half = viewSize * 0.5f;
    if (max - min <= viewSize) {
        return (min + max) * 0.5f;
    }
    return std::clamp(center, min + half, max - half);
}

// The area the view may show: the arena plus the margin on every side.
Arena ViewBounds(const Arena& arena, float margin) {
    return Arena{arena.min - Vec2{margin, margin}, arena.max + Vec2{margin, margin}};
}

} // namespace

Vec2 ClampViewToArena(Vec2 center, Vec2 viewSize, const Arena& arena) {
    return {ClampViewAxis(center.x, viewSize.x, arena.min.x, arena.max.x),
            ClampViewAxis(center.y, viewSize.y, arena.min.y, arena.max.y)};
}

FollowCamera FollowCameraAt(Vec2 target, Vec2 viewSize, const Arena& arena, const FollowCameraStats& stats) {
    const Vec2 center = ClampViewToArena(target, viewSize, ViewBounds(arena, stats.wallMargin));
    return FollowCamera{.center = center, .previousCenter = center, .viewSize = viewSize, .stats = stats};
}

void StepFollowCamera(FollowCamera& camera, Vec2 target, Vec2 targetVelocity, const Arena& arena, float dt) {
    // The goal is clamped, not the result. The current center is already inside the allowed range, and
    // a blend of two points inside it stays inside, so the camera slows down into the edge instead of
    // overshooting and snapping back.
    const Vec2 goal = ClampViewToArena(target + targetVelocity * camera.stats.lookAhead, camera.viewSize,
                                       ViewBounds(arena, camera.stats.wallMargin));

    // 1 - exp(-k * dt) is the fraction of the gap closed in one step. Unlike a fixed fraction per step,
    // it means the same follow speed if the simulation rate ever changes.
    const float blend = 1.0f - std::exp(-camera.stats.sharpness * dt);

    camera.previousCenter = camera.center;
    camera.center = Lerp(camera.center, goal, blend);
}

} // namespace horde
