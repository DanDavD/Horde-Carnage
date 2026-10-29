#pragma once

#include <optional>
#include <span>

#include "core/Arena.h"
#include "core/Player.h"
#include "core/Vec2.h"
#include "core/World.h"

namespace horde {

/// A circular body that stops a laser sight ray.
struct LaserTarget {
    Vec2 center;
    float radius = 0.0f;
};

/// The segment of the hunter's laser sight from the gun barrel to where the next shot stops.
struct LaserSight {
    Vec2 start;
    Vec2 end;
    bool hitsTarget = false; ///< Stopped on a living target before the wall or range cap.
    bool hitsWall = false;   ///< Stopped on an arena wall before any target or the range cap.
};

/// Whether the hunter has their gun up and ready to project a laser sight: alive, not dashing, and not
/// drinking a flask.
[[nodiscard]] bool CanProjectLaserSight(const Player& player);

/// Traces a ray from `muzzle` along `direction` up to `maxRange`, stopping at the nearest target in
/// `targets` or the nearest wall of `arena`, in the same priority order as `StepBullets`.
[[nodiscard]] LaserSight TraceLaserSight(Vec2 muzzle, Vec2 direction, float maxRange, const Arena& arena,
                                         std::span<const LaserTarget> targets);

/// Traces the hunter's laser sight in `world`, interpolated by `alpha` and optionally overridden with a
/// live frame `aimDirection` so high-refresh rendering stays locked to the crosshair.
[[nodiscard]] std::optional<LaserSight> TraceWorldLaserSight(const World& world, float alpha = 1.0f,
                                                             std::optional<Vec2> aimDirection = std::nullopt);

} // namespace horde
