#include "core/LaserSight.h"

#include <algorithm>
#include <array>

#include "core/Boss.h"
#include "core/Collision.h"
#include "core/Dash.h"
#include "core/Flasks.h"
#include "core/Health.h"
#include "core/Loadout.h"
#include "core/Weapon.h"

namespace horde {

bool CanProjectLaserSight(const Player& player) {
    return !IsDead(player.health) && !IsDashing(player.dash) && !IsDrinking(player.flasks);
}

LaserSight TraceLaserSight(Vec2 muzzle, Vec2 direction, float maxRange, const Arena& arena,
                           std::span<const LaserTarget> targets) {
    if (LengthSquared(direction) <= 1e-6f || maxRange <= 0.0f) {
        return LaserSight{.start = muzzle, .end = muzzle};
    }
    const Vec2 dir = Normalized(direction);

    float wallReach = maxRange;
    bool hitsWall = false;
    const auto clipAxis = [&](float start, float step, float low, float high) {
        if (step > 0.0f) {
            const float toWall = std::max((high - start) / step, 0.0f);
            if (toWall <= wallReach) {
                wallReach = toWall;
                hitsWall = true;
            }
        } else if (step < 0.0f) {
            const float toWall = std::max((low - start) / step, 0.0f);
            if (toWall <= wallReach) {
                wallReach = toWall;
                hitsWall = true;
            }
        }
    };
    clipAxis(muzzle.x, dir.x, arena.min.x, arena.max.x);
    clipAxis(muzzle.y, dir.y, arena.min.y, arena.max.y);
    if (!Contains(arena, muzzle)) {
        wallReach = 0.0f;
        hitsWall = true;
    }

    const Vec2 fullEnd = muzzle + dir * maxRange;
    float targetFraction = 2.0f;
    for (const LaserTarget& target : targets) {
        if (const std::optional<float> fraction = SweepCircle(muzzle, fullEnd, 0.0f, target.center, target.radius)) {
            targetFraction = std::min(targetFraction, *fraction);
        }
    }

    if (targetFraction <= 1.0f) {
        const float targetReach = targetFraction * maxRange;
        if (!hitsWall || targetReach <= wallReach) {
            return LaserSight{.start = muzzle, .end = muzzle + dir * targetReach, .hitsTarget = true};
        }
    }
    if (hitsWall) {
        return LaserSight{.start = muzzle, .end = muzzle + dir * wallReach, .hitsWall = true};
    }
    return LaserSight{.start = muzzle, .end = fullEnd};
}

std::optional<LaserSight> TraceWorldLaserSight(const World& world, float alpha,
                                               std::optional<Vec2> aimDirection) {
    const Player& player = world.player;
    if (!CanProjectLaserSight(player)) {
        return std::nullopt;
    }
    const Vec2 facing = aimDirection ? *aimDirection : player.facing;
    if (LengthSquared(facing) <= 1e-6f) {
        return std::nullopt;
    }
    const Vec2 dir = Normalized(facing);

    const Vec2 playerPos = Lerp(player.motion.previousPosition, player.motion.position, alpha);
    const WeaponStats& weapon = Equipped(player.loadout).stats;
    const Vec2 muzzle = MuzzlePoint(playerPos, dir, player.radius, weapon);

    constexpr std::size_t kMaxLaserTargets = kMaxEnemies + 1;
    std::array<LaserTarget, kMaxLaserTargets> targets{};
    std::size_t count = 0;

    if (!IsDead(world.boss.health) && !IsAirborne(world.boss)) {
        const Vec2 bossPos = Lerp(world.boss.motion.previousPosition, world.boss.motion.position, alpha);
        targets[count++] = LaserTarget{.center = bossPos, .radius = world.boss.radius};
    }
    for (const Enemy& enemy : world.enemies) {
        if (IsDead(enemy.health) || count >= targets.size()) {
            continue;
        }
        const Vec2 enemyPos = Lerp(enemy.motion.previousPosition, enemy.motion.position, alpha);
        targets[count++] = LaserTarget{.center = enemyPos, .radius = enemy.radius};
    }

    return TraceLaserSight(muzzle, dir, weapon.range, world.arena,
                           std::span<const LaserTarget>(targets.data(), count));
}

} // namespace horde
