#pragma once

#include <optional>

#include "app/render/Art.h"
#include "core/Particle.h"
#include "core/World.h"

namespace horde {

/// The whole world, in world space, in the order that keeps everything readable: the arena and the shadows
/// on it, the torchlight, settled floor debris, the warnings and marks on the floor, dust, the bodies, the
/// hunter's laser sight, the projectiles, the player, airborne sparks and shards, and a boss in the air over
/// all of it. `alpha` blends between the last two fixed steps. A dead boss plays its death `bossSinceDied`
/// seconds in.
void DrawWorld(const World& world, const ParticlePool& particles, float alpha, const Art& art,
               float bossSinceDied, bool showAimSight = false,
               std::optional<Vec2> aimDirection = std::nullopt);

} // namespace horde
