#include "core/Dash.h"

#include <algorithm>

namespace horde {

bool TryStartDash(Dash& dash, Vec2 origin, Vec2 direction) {
    if (dash.cooldownSeconds > 0.0f || LengthSquared(direction) == 0.0f) {
        return false;
    }
    dash.direction = Normalized(direction);
    dash.origin = origin;
    dash.activeSeconds = dash.stats.duration;
    dash.invulnerableSeconds = dash.stats.invulnerability;
    // The cooldown counts from the start, so it covers the burst and the rest after it.
    dash.cooldownSeconds = dash.stats.duration + dash.stats.cooldown;
    return true;
}

Vec2 StepDash(Dash& dash, float dt) {
    dash.invulnerableSeconds = std::max(dash.invulnerableSeconds - dt, 0.0f);
    if (!IsDashing(dash)) {
        return {};
    }
    const float moving = std::min(dt, dash.activeSeconds);
    dash.activeSeconds -= moving;
    const float speed = dash.stats.distance / dash.stats.duration;
    return dash.direction * (speed * moving);
}

void TickDashCooldown(Dash& dash, float dt) {
    dash.cooldownSeconds = std::max(dash.cooldownSeconds - dt, 0.0f);
}

} // namespace horde
