#pragma once

#include <cmath>

namespace horde {

/// A 2D vector in world units for positions, velocities and directions.
///
/// horde_core has its own type instead of raylib's Vector2 so the pure logic and its tests
/// never include raylib. The app converts at the edge, where it draws.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2& operator+=(Vec2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vec2& operator-=(Vec2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vec2& operator*=(float scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    friend constexpr Vec2 operator+(Vec2 a, Vec2 b) { return a += b; }
    friend constexpr Vec2 operator-(Vec2 a, Vec2 b) { return a -= b; }
    friend constexpr Vec2 operator*(Vec2 v, float scale) { return v *= scale; }
    friend constexpr Vec2 operator*(float scale, Vec2 v) { return v *= scale; }
    friend constexpr bool operator==(Vec2 a, Vec2 b) = default;
};

[[nodiscard]] constexpr float LengthSquared(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

[[nodiscard]] inline float Length(Vec2 v) {
    return std::sqrt(LengthSquared(v));
}

/// Returns `v` shortened to `maxLength` if it is longer, unchanged otherwise.
/// Movement input goes through this so a diagonal on the keyboard is not faster than a straight line.
[[nodiscard]] inline Vec2 ClampLength(Vec2 v, float maxLength) {
    const float lengthSquared = LengthSquared(v);
    if (lengthSquared <= maxLength * maxLength) {
        return v;
    }
    return v * (maxLength / std::sqrt(lengthSquared));
}

/// Moves `current` toward `target` by at most `maxDelta`, and never past it.
[[nodiscard]] inline Vec2 MoveTowards(Vec2 current, Vec2 target, float maxDelta) {
    const Vec2 toTarget = target - current;
    const float distance = Length(toTarget);
    if (distance <= maxDelta || distance == 0.0f) {
        return target;
    }
    return current + toTarget * (maxDelta / distance);
}

/// Linear interpolation: `t` = 0 gives `a`, `t` = 1 gives `b`.
[[nodiscard]] constexpr Vec2 Lerp(Vec2 a, Vec2 b, float t) {
    return a + (b - a) * t;
}

} // namespace horde
