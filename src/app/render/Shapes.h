#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

#include <raylib.h>

#include "app/RaylibConvert.h"
#include "core/Vec2.h"

namespace horde {

// Shapes raylib does not have, for the drawings made in code: the hunter's cape and hat, and the hairy
// bodies of the boss and its children.

inline constexpr float kPi = std::numbers::pi_v<float>;

/// The angle of `v`, in radians.
[[nodiscard]] inline float Angle(Vec2 v) {
    return std::atan2(v.y, v.x);
}

/// A number from 0 to 1 that looks random but is always the same for the same `n`, so a drawing can vary
/// from lock to lock without storing anything.
[[nodiscard]] inline float Hash(float n) {
    const float x = std::sin(n * 12.9898f) * 43758.5453f;
    return x - std::floor(x);
}

/// How far `angle` points along `direction` (also an angle): 1 straight along it, 0 at right angles or
/// further away, rising steeply with `steepness`, so a shape bulges out only around that direction.
[[nodiscard]] inline float Bulge(float angle, float direction, float steepness) {
    return std::pow(std::max(std::cos(angle - direction), 0.0f), steepness);
}

/// A closed shape around `center` whose distance from it at each angle is `radiusAt(angle)`, with every
/// point then moved by `warp` (an offset from the centre in, an offset out), which squashes or stretches
/// it. raylib only draws triangles that go round counterclockwise on screen, so the points go round that
/// way, as its own circles do.
template <int Points, typename RadiusAt, typename Warp>
void DrawRadialShape(Vec2 center, RadiusAt radiusAt, Warp warp, Color color) {
    std::array<Vector2, Points + 2> fan{};
    fan[0] = ToRaylib(center);
    for (int i = 0; i <= Points; ++i) {
        const float angle = -2.0f * kPi * static_cast<float>(i) / Points;
        const Vec2 out{std::cos(angle), std::sin(angle)};
        fan[static_cast<std::size_t>(i) + 1] = ToRaylib(center + warp(out * radiusAt(angle)));
    }
    DrawTriangleFan(fan.data(), static_cast<int>(fan.size()), color);
}

/// The same, without a warp.
template <int Points, typename RadiusAt>
void DrawRadialShape(Vec2 center, RadiusAt radiusAt, Color color) {
    DrawRadialShape<Points>(center, radiusAt, [](Vec2 offset) { return offset; }, color);
}

/// An ellipse with its `alongRadius` axis on `along`, a unit vector: raylib has no rotated ellipse.
void DrawOrientedEllipse(Vec2 center, Vec2 along, float alongRadius, float acrossRadius, Color color);

/// A triangle, whichever way its corners go round. raylib only draws the ones that go one way.
void DrawTriangleAnyWinding(Vec2 a, Vec2 b, Vec2 c, Color color);

} // namespace horde
