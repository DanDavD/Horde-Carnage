#include "core/MenuRepeat.h"

#include <algorithm>
#include <cmath>

namespace horde {

namespace {

constexpr float kMaxFrameStep = 0.25f;

MenuRepeatStep ResolveDirection(const MenuRepeat& current, Vec2 direction, const MenuRepeatStats& stats) {
    const float absX = std::abs(direction.x);
    const float absY = std::abs(direction.y);

    if (current.heldMove != 0) {
        // Rolling cleanly onto the horizontal axis switches from moving between rows to adjusting.
        if (absX >= stats.engageThreshold && absX > absY) {
            return MenuRepeatStep{.adjust = direction.x < 0.0f ? -1 : 1};
        }
        // Snapping across center to the opposite vertical direction starts a fresh move that way.
        if (direction.y * static_cast<float>(current.heldMove) <= -stats.engageThreshold) {
            return MenuRepeatStep{.move = -current.heldMove};
        }
        // Hysteresis keeps the axis engaged down to releaseThreshold so thumb tremor never double-ticks.
        if (direction.y * static_cast<float>(current.heldMove) >= stats.releaseThreshold) {
            return MenuRepeatStep{.move = current.heldMove};
        }
        return {};
    }

    if (current.heldAdjust != 0) {
        if (absY >= stats.engageThreshold && absY >= absX) {
            return MenuRepeatStep{.move = direction.y < 0.0f ? -1 : 1};
        }
        if (direction.x * static_cast<float>(current.heldAdjust) <= -stats.engageThreshold) {
            return MenuRepeatStep{.adjust = -current.heldAdjust};
        }
        if (direction.x * static_cast<float>(current.heldAdjust) >= stats.releaseThreshold) {
            return MenuRepeatStep{.adjust = current.heldAdjust};
        }
        return {};
    }

    if (absX < stats.engageThreshold && absY < stats.engageThreshold) {
        return {};
    }
    // Vertical wins ties so an exact diagonal scrolls items rather than nudging a setting.
    if (absY >= absX) {
        return MenuRepeatStep{.move = direction.y < 0.0f ? -1 : 1};
    }
    return MenuRepeatStep{.adjust = direction.x < 0.0f ? -1 : 1};
}

} // namespace

void LatchMenuRepeat(MenuRepeat& repeat, Vec2 direction, const MenuRepeatStats& stats) {
    const MenuRepeat idle{};
    const MenuRepeatStep desired = ResolveDirection(idle, direction, stats);
    repeat.heldMove = desired.move;
    repeat.heldAdjust = desired.adjust;
    repeat.secondsUntilNext = (desired.move != 0 || desired.adjust != 0) ? stats.initialDelay : 0.0f;
}

MenuRepeatStep StepMenuRepeat(MenuRepeat& repeat, Vec2 direction, float dt, const MenuRepeatStats& stats) {
    const MenuRepeatStep desired = ResolveDirection(repeat, direction, stats);
    if (desired.move == 0 && desired.adjust == 0) {
        repeat = {};
        return {};
    }

    if (desired.move != repeat.heldMove || desired.adjust != repeat.heldAdjust) {
        repeat.heldMove = desired.move;
        repeat.heldAdjust = desired.adjust;
        repeat.secondsUntilNext = stats.initialDelay;
        return desired;
    }

    const float clampedDt = std::clamp(dt, 0.0f, kMaxFrameStep);
    repeat.secondsUntilNext -= clampedDt;
    if (repeat.secondsUntilNext <= 0.0f) {
        repeat.secondsUntilNext = std::max(repeat.secondsUntilNext + stats.repeatInterval, 0.0f);
        return desired;
    }
    return {};
}

} // namespace horde
