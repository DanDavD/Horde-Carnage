#include "core/Flasks.h"

namespace horde {

bool TryStartDrinking(Flasks& flasks) {
    if (flasks.charges <= 0 || IsDrinking(flasks)) {
        return false;
    }
    --flasks.charges;
    flasks.drinkingSeconds = flasks.stats.drinkSeconds;
    return true;
}

int StepFlasks(Flasks& flasks, float dt) {
    if (!IsDrinking(flasks)) {
        return 0;
    }
    flasks.drinkingSeconds -= dt;
    if (flasks.drinkingSeconds > 0.0f) {
        return 0;
    }
    flasks.drinkingSeconds = 0.0f;
    return flasks.stats.heal;
}

void InterruptDrinking(Flasks& flasks) {
    flasks.drinkingSeconds = 0.0f;
}

} // namespace horde
