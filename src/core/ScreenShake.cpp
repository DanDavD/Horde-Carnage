#include "core/ScreenShake.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace horde {

namespace {

// Each axis and the turn read their own channel, so they do not move in step.
constexpr std::uint32_t kChannelX = 0;
constexpr std::uint32_t kChannelY = 1;
constexpr std::uint32_t kChannelTurn = 2;

// A number from -1 to 1 for each whole point on the noise's line, always the same for the same point. An
// integer hash rather than the fight's Random: the shake must not take numbers from the fight.
float Lattice(std::int32_t point, std::uint32_t channel) {
    std::uint32_t x = static_cast<std::uint32_t>(point) * 0x9E3779B1u ^ (channel + 1u) * 0x85EBCA77u;
    x ^= x >> 15;
    x *= 0x2C1B3C6Du;
    x ^= x >> 12;
    x *= 0x297A2D39u;
    x ^= x >> 15;
    return static_cast<float>(x) / 4294967295.0f * 2.0f - 1.0f;
}

// Smooth noise: the lattice points blended with a curve that is flat at each point, so the shake changes
// direction without a kink.
float SmoothNoise(float t, std::uint32_t channel) {
    const float cell = std::floor(t);
    const float f = t - cell;
    const float blend = f * f * (3.0f - 2.0f * f);
    const auto point = static_cast<std::int32_t>(cell);
    const float a = Lattice(point, channel);
    return a + (Lattice(point + 1, channel) - a) * blend;
}

} // namespace

void AddTrauma(ScreenShake& shake, float amount) {
    shake.trauma = std::clamp(shake.trauma + amount, 0.0f, 1.0f);
}

void StepScreenShake(ScreenShake& shake, float dt) {
    shake.trauma = std::max(shake.trauma - shake.stats.decayPerSecond * dt, 0.0f);
    // The clock starts again once the shake has settled, so it never grows large enough to lose precision.
    shake.seconds = shake.trauma > 0.0f ? shake.seconds + dt : 0.0f;
}

ShakeOffset CurrentShake(const ScreenShake& shake) {
    const float strength = shake.trauma * shake.trauma;
    if (strength <= 0.0f) {
        return {};
    }
    const ScreenShakeStats& stats = shake.stats;
    const float t = shake.seconds * stats.frequency;
    const Vec2 direction{SmoothNoise(t, kChannelX), SmoothNoise(t, kChannelY)};
    return ShakeOffset{.offset = direction * (stats.maxOffset * strength),
                       .turnDegrees = SmoothNoise(t, kChannelTurn) * stats.maxTurnDegrees * strength};
}

} // namespace horde
