#include "core/GamepadAim.h"

#include <algorithm>
#include <cmath>

namespace horde {

Vec2 ApplyAimAssist(Vec2 playerPosition, Vec2 aimDirection, std::span<const Vec2> targets,
                    const AimAssistStats& stats) {
    if (LengthSquared(aimDirection) < 0.001f) {
        return aimDirection;
    }

    const Vec2 normAim = Normalized(aimDirection);
    if (targets.empty() || stats.maxAngleRadians <= 0.0f || stats.maxStrength <= 0.0f) {
        return normAim;
    }

    const float minCos = std::cos(stats.maxAngleRadians);
    float bestCos = minCos;
    Vec2 bestTargetDir{};
    bool found = false;

    for (const Vec2& target : targets) {
        const Vec2 toTarget = target - playerPosition;
        const float distSq = LengthSquared(toTarget);
        if (distSq < 1.0f || distSq > stats.maxRange * stats.maxRange) {
            continue;
        }

        const float dist = std::sqrt(distSq);
        const Vec2 targetDir = toTarget * (1.0f / dist);
        const float dot = Dot(normAim, targetDir);

        if (dot > bestCos) {
            bestCos = dot;
            bestTargetDir = targetDir;
            found = true;
        }
    }

    if (!found) {
        return normAim;
    }

    const float angle = std::acos(std::clamp(bestCos, -1.0f, 1.0f));
    const float t = std::clamp(1.0f - (angle / stats.maxAngleRadians), 0.0f, 1.0f);
    // Smoothstep falloff so the pull tapers off gently at the edge of the cone.
    const float smoothT = t * t * (3.0f - 2.0f * t);
    const float pull = std::clamp(stats.maxStrength * smoothT, 0.0f, 1.0f);

    return Normalized(Lerp(normAim, bestTargetDir, pull));
}

Vec2 AssistAimInWorld(const World& world, Vec2 aimDirection, const AimAssistStats& stats) {
    if (LengthSquared(aimDirection) < 0.001f) {
        return aimDirection;
    }

    // Fixed capacity array on the stack to avoid heap allocation per step.
    constexpr std::size_t kMaxCandidateTargets = 32;
    Vec2 candidates[kMaxCandidateTargets];
    std::size_t count = 0;

    if (!IsBossDefeated(world)) {
        candidates[count++] = world.boss.motion.position;
    }

    for (const Enemy& enemy : world.enemies) {
        if (count >= kMaxCandidateTargets) {
            break;
        }
        if (enemy.health.current > 0) {
            candidates[count++] = enemy.motion.position;
        }
    }

    return ApplyAimAssist(world.player.motion.position, aimDirection,
                          std::span<const Vec2>(candidates, count), stats);
}

Vec2 GamepadAimPoint(Vec2 playerPosition, Vec2 stickDirection, Vec2 fallbackDirection,
                     float crosshairDistance) {
    Vec2 direction = LengthSquared(stickDirection) > 0.001f ? Normalized(stickDirection) : fallbackDirection;
    if (LengthSquared(direction) < 0.001f) {
        direction = Vec2{0.0f, -1.0f};
    } else {
        direction = Normalized(direction);
    }
    return playerPosition + direction * crosshairDistance;
}

Vec2 StepGamepadAim(GamepadAim& aim, const World& world, Vec2 rightStick, bool firing,
                    const AimAssistStats& stats) {
    if (LengthSquared(rightStick) > 0.001f) {
        aim.rawDirection = Normalized(rightStick);
        aim.direction = AssistAimInWorld(world, aim.rawDirection, stats);
    } else if (firing) {
        aim.direction = AssistAimInWorld(world, aim.rawDirection, stats);
    }
    return GamepadAimPoint(world.player.motion.position, {}, aim.direction, stats.crosshairDistance);
}

} // namespace horde
