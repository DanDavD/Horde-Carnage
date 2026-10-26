#pragma once

#include "app/render/Art.h"
#include "core/World.h"

namespace horde {

/// The whole world, in world space, in the order that keeps everything readable: the arena and the shadows
/// on it, the torchlight, the warnings and marks on the floor, the bodies, the projectiles, the player, and
/// a boss in the air over all of it. `alpha` blends between the last two fixed steps.
void DrawWorld(const World& world, float alpha, const Art& art);

} // namespace horde
