#pragma once

#include "core/Movement.h"
#include "core/Vec2.h"

namespace horde {

/// The rectangle the fight happens in, in world units. y grows downward, like on screen.
struct Arena {
    Vec2 min;
    Vec2 max;
};

[[nodiscard]] constexpr Vec2 Center(const Arena& arena) {
    return Lerp(arena.min, arena.max, 0.5f);
}

[[nodiscard]] constexpr bool Contains(const Arena& arena, Vec2 point) {
    return point.x >= arena.min.x && point.x <= arena.max.x && point.y >= arena.min.y && point.y <= arena.max.y;
}

/// Pushes a circle of `radius` back inside the arena walls and cancels the part of its velocity
/// that points into a wall it touches. The rest of the velocity is kept, so a mover slides along
/// a wall instead of sticking to it.
void ConfineToArena(Motion& motion, float radius, const Arena& arena);

} // namespace horde
