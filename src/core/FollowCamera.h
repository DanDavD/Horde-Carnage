#pragma once

#include "core/Arena.h"
#include "core/Vec2.h"

namespace horde {

/// How the camera chases its target.
///
/// A camera that follows exponentially trails a moving target by `speed / sharpness`. With the
/// defaults the player at top speed (300) would trail by 37.5 units, and the look-ahead of
/// 300 * 0.25 = 75 more than covers it, so the view ends up about 37 units ahead of the player.
struct FollowCameraStats {
    float sharpness = 8.0f;  ///< Per second. Higher follows tighter. Time constant is 1 / sharpness.
    float lookAhead = 0.25f; ///< Seconds of the target's velocity to aim ahead, so the player sees where they go.
    /// How far past each wall the view may reach. Without it the screen edge stops exactly on the arena
    /// edge, the wall (drawn just outside) is never on screen, and the player hits an invisible wall.
    float wallMargin = 64.0f;
};

/// A 2D camera that follows a target and never shows more than `wallMargin` past the arena walls.
///
/// It lives in world units and knows nothing about raylib. The app turns `Lerp(previousCenter, center, alpha)`
/// into a raylib `Camera2D` when it draws.
struct FollowCamera {
    Vec2 center;
    /// Center before the last fixed step, for the same interpolation the player uses.
    Vec2 previousCenter;
    Vec2 viewSize; ///< How much of the world fits on screen, in world units.
    FollowCameraStats stats;
};

/// Where the center of a view of `viewSize` has to be so the view stays inside `arena`. On an axis
/// where the arena is smaller than the view, the view is centred on the arena instead.
[[nodiscard]] Vec2 ClampViewToArena(Vec2 center, Vec2 viewSize, const Arena& arena);

/// A camera already settled on `target`, so the first frame does not slide in from somewhere else.
[[nodiscard]] FollowCamera FollowCameraAt(Vec2 target, Vec2 viewSize, const Arena& arena,
                                          const FollowCameraStats& stats = {});

/// Where `camera.center` lands after one fixed step of `dt` seconds toward `target` plus look-ahead,
/// without mutating `camera`. Used when aiming through a screen-space crosshair before the camera steps.
[[nodiscard]] Vec2 NextFollowCameraCenter(const FollowCamera& camera, Vec2 target, Vec2 targetVelocity,
                                          const Arena& arena, float dt);

/// Moves the camera one fixed step of `dt` seconds toward `target` plus a look-ahead along `targetVelocity`.
void StepFollowCamera(FollowCamera& camera, Vec2 target, Vec2 targetVelocity, const Arena& arena, float dt);

} // namespace horde
