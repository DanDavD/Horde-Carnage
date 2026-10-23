#include "core/Collision.h"

#include <cassert>
#include <cmath>

namespace horde {

bool AreTouching(Vec2 a, float radiusA, Vec2 b, float radiusB) {
    const float reach = radiusA + radiusB + kContactSlack;
    return LengthSquared(b - a) <= reach * reach;
}

std::optional<float> SweepCircle(Vec2 from, Vec2 to, float radius, Vec2 center, float otherRadius) {
    // Solve |from + t * move - center| = radius + otherRadius for the smallest t in [0, 1].
    const Vec2 move = to - from;
    const Vec2 offset = from - center;
    const float reach = radius + otherRadius;
    const float c = LengthSquared(offset) - reach * reach;
    if (c <= 0.0f) {
        return 0.0f;
    }
    const float a = LengthSquared(move);
    const float b = 2.0f * Dot(offset, move);
    const float discriminant = b * b - 4.0f * a * c;
    if (a == 0.0f || discriminant < 0.0f) {
        return std::nullopt;
    }
    const float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0.0f || t > 1.0f) {
        return std::nullopt;
    }
    return t;
}

float PushShare(float weightA, float weightB) {
    assert(weightA > 0.0f && weightB > 0.0f && "collision weights must be positive");
    return weightB / (weightA + weightB);
}

bool SeparateCircles(Motion& a, float radiusA, Motion& b, float radiusB, float shareA) {
    const Vec2 delta = b.position - a.position;
    const float contact = radiusA + radiusB;
    const float distanceSquared = LengthSquared(delta);
    if (distanceSquared >= contact * contact) {
        return false;
    }

    // Two centres on the same point have no line between them. Any fixed direction works, and a fixed one
    // keeps the simulation deterministic.
    const float distance = std::sqrt(distanceSquared);
    const Vec2 normal = distance > 0.0f ? delta * (1.0f / distance) : Vec2{1.0f, 0.0f};

    const float overlap = contact - distance;
    const float shareB = 1.0f - shareA;
    a.position -= normal * (overlap * shareA);
    b.position += normal * (overlap * shareB);

    // Moving the positions alone is not enough. A hairball blocked by another one would keep its full speed
    // into it, push again every step, and the crowd would slowly shove the player around. Removing the
    // closing speed is the same idea as ConfineToArena cancelling the velocity into a wall, where the wall
    // is a body of infinite weight.
    const float closingSpeed = Dot(b.velocity - a.velocity, normal);
    if (closingSpeed < 0.0f) {
        a.velocity += normal * (closingSpeed * shareA);
        b.velocity -= normal * (closingSpeed * shareB);
    }
    return true;
}

} // namespace horde
