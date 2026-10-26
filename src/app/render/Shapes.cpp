#include "app/render/Shapes.h"

namespace horde {

void DrawOrientedEllipse(Vec2 center, Vec2 along, float alongRadius, float acrossRadius, Color color) {
    const float turn = Angle(along);
    DrawRadialShape<40>(
        center,
        [&](float angle) {
            const float c = std::cos(angle - turn) / alongRadius;
            const float s = std::sin(angle - turn) / acrossRadius;
            return 1.0f / std::sqrt(c * c + s * s);
        },
        color);
}

void DrawTriangleAnyWinding(Vec2 a, Vec2 b, Vec2 c, Color color) {
    const Vec2 ab = b - a;
    const Vec2 ac = c - a;
    // On screen y grows downward, so a positive cross product goes round clockwise to the eye, and raylib
    // only draws triangles that go round the other way.
    if (ab.x * ac.y - ab.y * ac.x > 0.0f) {
        DrawTriangle(ToRaylib(a), ToRaylib(c), ToRaylib(b), color);
    } else {
        DrawTriangle(ToRaylib(a), ToRaylib(b), ToRaylib(c), color);
    }
}

} // namespace horde
