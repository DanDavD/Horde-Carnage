#include "core/TrailingBar.h"

#include <algorithm>

namespace horde {

TrailingBar TrailingBarAt(int health) {
    return TrailingBar{.trail = static_cast<float>(health), .lastHealth = health};
}

void StepTrailingBar(TrailingBar& bar, int health, int maxHealth, float dt) {
    const auto now = static_cast<float>(health);
    if (health < bar.lastHealth) {
        // The trail had already caught up with the bar, so this hit opens a new run.
        if (bar.trail <= static_cast<float>(bar.lastHealth)) {
            bar.tally = 0;
        }
        bar.tally += bar.lastHealth - health;
        bar.holdSecondsLeft = bar.stats.holdSeconds;
    } else if (bar.holdSecondsLeft > 0.0f) {
        bar.holdSecondsLeft = std::max(bar.holdSecondsLeft - dt, 0.0f);
    } else {
        bar.trail -= bar.stats.drainShare * static_cast<float>(maxHealth) * dt;
    }
    bar.trail = std::max(bar.trail, now);
    if (bar.trail <= now && bar.holdSecondsLeft <= 0.0f) {
        bar.tally = 0;
    }
    bar.lastHealth = health;
}

bool IsTrailing(const TrailingBar& bar, int health) {
    return bar.trail > static_cast<float>(health);
}

} // namespace horde
