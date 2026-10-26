#pragma once

#include "core/World.h"

namespace horde {

/// The whole world, in world space, in the order that keeps everything readable: the arena, the warnings
/// and marks on the floor, the bodies, the projectiles, the player, and a boss in the air over all of it.
/// `alpha` blends between the last two fixed steps.
void DrawWorld(const World& world, float alpha);

} // namespace horde
