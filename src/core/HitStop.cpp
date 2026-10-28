#include "core/HitStop.h"

#include <algorithm>

namespace horde {

void StartHitStop(HitStop& stop, float seconds) {
    if (seconds > stop.secondsLeft) {
        stop.secondsLeft = seconds;
        stop.seconds = seconds;
    }
}

void StepHitStop(HitStop& stop, float realSeconds) {
    stop.secondsLeft = std::max(stop.secondsLeft - realSeconds, 0.0f);
}

float TimeScale(const HitStop& stop) {
    if (stop.secondsLeft <= 0.0f || stop.seconds <= 0.0f) {
        return 1.0f;
    }
    // Smoothstep from the hit to the end: slow at the start, so the hit has a moment to land, and flat at
    // the end, so the fight reaches full speed without a jolt.
    const float progress = 1.0f - stop.secondsLeft / stop.seconds;
    const float eased = progress * progress * (3.0f - 2.0f * progress);
    const float slowest = stop.stats.slowestScale;
    return slowest + (1.0f - slowest) * eased;
}

} // namespace horde
