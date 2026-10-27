#pragma once

#include <cstdint>

#include "core/Particle.h"
#include "core/Random.h"
#include "core/World.h"
#include "core/WorldEvent.h"

namespace horde {

/// Turns what happens in the fight into particles: one-off bursts from `WorldEvent`s, and steady streams
/// while the player dashes or the boss sheds hair in her second phase.
///
/// It owns both the pool and its own `Random`, separate from `World::random`, so throwing particles never
/// draws a number from the fight's generator or changes a replay's seed.
struct ParticleEmitter {
    ParticlePool particles;
    Random random = SeedRandom(0, 11);
    float dashDustSeconds = 0.0f;
    float bossHairSeconds = 0.0f;
};

/// A fresh emitter with its own random stream seeded from `seed`.
[[nodiscard]] ParticleEmitter SeedParticleEmitter(std::uint64_t seed = 0);

/// Empties the pool and resets the continuous timers, for a new attempt.
void ClearParticles(ParticleEmitter& emitter);

/// Spawns the burst for one `WorldEvent`: muzzle flash, smoke and casing on a shot; sparks on a shield;
/// dark blood and hair on an exposed hit; shards on a break; dust on a dash or landing; hair on a kill.
void EmitEventParticles(ParticleEmitter& emitter, const WorldEvent& event, const World& world);

/// Spawns the steady trails of a playing step: dust under a dash, and locks of hair falling from the boss
/// once she leaves her first phase, faster in a frenzy.
void EmitContinuousParticles(ParticleEmitter& emitter, const World& world, float dt);

} // namespace horde
