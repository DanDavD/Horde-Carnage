#pragma once

#include "app/render/Art.h"
#include "core/Particle.h"

namespace horde {

/// Casings, blood splats, locks of hair and shield chips that have come to rest on the stone floor. Drawn
/// right over the torchlight and under every telegraph, wave and body, so floor litter never hides a warning.
void DrawFloorParticles(const ParticlePool& particles);

/// Shadows under falling particles and clouds of stone dust, drawn over telegraphs and under the bodies so a
/// dash or a landing ring never covers the hunter, the boss or her strands.
void DrawUnderbodyParticles(const ParticlePool& particles, const Art& art);

/// Sparks, gunsmoke, and casings, drops, hair locks and shield shards still in the air, drawn over the bodies
/// on the floor so hits on the shield and the boss read on top of what they struck.
void DrawOverbodyParticles(const ParticlePool& particles, const Art& art);

} // namespace horde
