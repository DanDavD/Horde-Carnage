#pragma once

#include "app/render/Art.h"
#include "core/Particle.h"
#include "core/World.h"

namespace horde {

/// The whole world, in world space, in the order that keeps everything readable: the arena and the shadows
/// on it, the torchlight, settled floor debris, the warnings and marks on the floor, dust, the bodies, the
/// projectiles, the player, airborne sparks and shards, and a boss in the air over all of it. `alpha` blends
/// between the last two fixed steps.
void DrawWorld(const World& world, const ParticlePool& particles, float alpha, const Art& art);

} // namespace horde
