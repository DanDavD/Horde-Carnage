#include "core/SpawnMarker.h"

#include <algorithm>
#include <numbers>

namespace horde {

SummonPoints PlanSummon(Vec2 player, Vec2 boss, const Arena& arena, const SummonPlacement& placement,
                        float turn) {
    SummonPoints plan;
    const int count = std::clamp(placement.count, 0, static_cast<int>(kMaxSummonPoints));
    const float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(std::max(count, 1));
    const float margin = placement.wallMargin;
    for (int i = 0; i < count; ++i) {
        const Vec2 out = Rotated(Vec2{placement.distance, 0.0f}, turn + step * static_cast<float>(i));
        const Vec2 wanted = player + out;
        const Vec2 point{std::clamp(wanted.x, arena.min.x + margin, arena.max.x - margin),
                         std::clamp(wanted.y, arena.min.y + margin, arena.max.y - margin)};
        if (Length(point - player) >= placement.minPlayerDistance &&
            Length(point - boss) >= placement.minBossDistance) {
            plan.points[plan.count++] = point;
        }
    }
    return plan;
}

SpawnMarker MakeSpawnMarker(Vec2 position, float seconds) {
    return SpawnMarker{.position = position, .secondsLeft = seconds, .totalSeconds = seconds};
}

bool StepSpawnMarker(SpawnMarker& marker, float dt) {
    marker.secondsLeft -= dt;
    return marker.secondsLeft > 0.0f;
}

} // namespace horde
