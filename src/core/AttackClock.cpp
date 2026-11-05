#include "core/AttackClock.h"

namespace horde {

void TickClock(AttackClock& clock, float dt) {
    clock.seconds += dt;
}

void EnterStage(AttackClock& clock, AttackStage stage) {
    clock.stage = stage;
    clock.seconds = 0.0f;
}

bool HasLasted(const AttackClock& clock, float length) {
    return clock.seconds >= length;
}

bool IsBeatDue(const AttackClock& clock, int index, float interval) {
    return clock.seconds >= static_cast<float>(index) * interval;
}

bool HasLastedPastBeats(const AttackClock& clock, int count, float interval, float tail) {
    const float lastBeat = static_cast<float>(count - 1) * interval;
    return clock.seconds >= lastBeat + tail;
}

} // namespace horde
