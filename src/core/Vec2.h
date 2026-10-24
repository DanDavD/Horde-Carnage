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

/// Positive when `a` and `b` point roughly the same way, negative when they point apart, zero at 90 degrees.
[[nodiscard]] constexpr float Dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

/// Returns `v` scaled to length 1. A zero vector has no direction, so it stays zero.
[[nodiscard]] inline Vec2 Normalized(Vec2 v) {
    const float length = Length(v);
    if (length == 0.0f) {
        return {};
    }
    return v * (1.0f / length);
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

/// Returns `v` turned by `radians`. Positive angles turn from +x toward +y, which is clockwise on screen,
/// where y points down. The same convention as `std::atan2(v.y, v.x)` and raylib's angles.
[[nodiscard]] inline Vec2 Rotated(Vec2 v, float radians) {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {v.x * cosine - v.y * sine, v.x * sine + v.y * cosine};
}

/// Linear interpolation: `t` = 0 gives `a`, `t` = 1 gives `b`.
[[nodiscard]] constexpr Vec2 Lerp(Vec2 a, Vec2 b, float t) {
    return a + (b - a) * t;
}

} // namespace horde
