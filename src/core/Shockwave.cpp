#include "core/Shockwave.h"

namespace horde {

Shockwave MakeShockwave(Vec2 center, float radius, const ShockwaveStats& stats) {
    return Shockwave{.center = center, .radius = radius, .stats = stats};
}

bool StepShockwave(Shockwave& wave, float dt) {
    wave.radius += wave.stats.speed * dt;
    return wave.radius <= wave.stats.reach;
}

bool ShockwaveTouches(const Shockwave& wave, Vec2 position, float radius) {
    // The band runs from `radius - thickness` to `radius` around the centre. A circle overlaps it when its
    // nearest point is inside the front and its farthest point is past the back.
    const float distance = Length(position - wave.center);
    return distance - radius <= wave.radius && distance + radius >= wave.radius - wave.stats.thickness;
}

} // namespace horde
