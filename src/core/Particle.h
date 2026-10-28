#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Arena.h"
#include "core/ObjectPool.h"
#include "core/Vec2.h"

namespace horde {

/// What a particle is. It decides how the particle is drawn; how it moves is in its numbers.
enum class ParticleKind : std::uint8_t {
    Spark,  ///< A short, bright streak along its flight: from a muzzle, or a bullet striking the shield.
    Smoke,  ///< A grey puff that grows and fades: a shot's.
    Dust,   ///< Stone dust thrown up by a dash, a landing or a crash, that spreads out and settles.
    Casing, ///< A spent brass casing that bounces and then lies on the floor for a while.
    Blood,  ///< A dark drop that falls to the floor and stays there a moment.
    Hair,   ///< A lock of hair that tumbles, drifts down and lies on the floor.
    Shard,  ///< A piece of a broken shield, spinning as it falls.
};

/// Which colour a kind comes in, for the kinds that come in more than one.
enum class ParticleTint : std::uint8_t {
    Plain,    ///< The kind's own colour: a muzzle's sparks, the player's blood, stone dust.
    Shell,    ///< The blue shield.
    Barrier,  ///< The orange shield.
    Boss,     ///< The Matriarch's dark hair and blood.
    Hairball, ///< Her children's hair, lighter and browner.
};

/// One particle: a spark, a puff, a casing, a drop, a lock of hair. Only something to look at. Nothing in
/// the fight reads a particle, so they live in the game, not in the `World`.
///
/// It moves across the floor (`position`, `velocity`) and, apart from that, up and down above it
/// (`height`, `lift`), so a falling particle keeps its spot on the floor and its shadow stays under it.
/// With `gravity` it falls back down, bounces off the floor with `bounce` of its speed, and once it has no
/// bounce left it lies there until its life runs out. Without gravity it floats, like smoke.
struct Particle {
    ParticleKind kind = ParticleKind::Dust;
    ParticleTint tint = ParticleTint::Plain;
    Vec2 position;        ///< On the floor, in world units.
    Vec2 velocity;        ///< Across the floor, in units per second.
    float drag = 0.0f;    ///< How fast `velocity` fades, per second: it keeps e^-drag of itself each second.
    float height = 0.0f;  ///< Above the floor, in units. Drawn as a shift up the screen.
    float lift = 0.0f;    ///< Speed upward, in units per second.
    float gravity = 0.0f; ///< Pull downward, in units per second squared. At 0 it floats and never lands.
    float bounce = 0.0f;  ///< Share of its speed it keeps when it hits the floor. At 0 it sticks where it lands.
    float angle = 0.0f;   ///< Radians, for the kinds that have a shape to turn.
    float spin = 0.0f;    ///< Radians per second. Each bounce takes from it like from the speed.
    float size = 1.0f;    ///< In units: a radius or a length, depending on the kind.
    float growth = 0.0f;  ///< Size gained per second: smoke and dust spread out as they go.
    float seconds = 1.0f;     ///< How long it lives in all.
    float secondsLeft = 1.0f; ///< Until it is gone.
};

/// Pool size. A burst is a handful to a few dozen particles, and most live under a second; casings, drops
/// and hair lie on the floor for a few. The SMG on the boss with her hair falling in a frenzy keeps about
/// 200 alive, so 512 leaves room for a break and a landing on top. A full pool drops new particles, which
/// only means a burst has a few less.
inline constexpr std::size_t kMaxParticles = 512;
using ParticlePool = ObjectPool<Particle, kMaxParticles>;

/// A particle with its full life ahead of it: `secondsLeft` set to `seconds`.
[[nodiscard]] constexpr Particle WithLife(Particle particle, float seconds) {
    particle.seconds = seconds;
    particle.secondsLeft = seconds;
    return particle;
}

/// Whether a particle has fallen and lies still on the floor: it has gravity, no height and no bounce left.
[[nodiscard]] constexpr bool IsOnFloor(const Particle& particle) {
    return particle.gravity > 0.0f && particle.height <= 0.0f && particle.lift == 0.0f;
}

/// How much of its life is left, from 1 when it is born to 0 when it is gone. The drawing fades by it.
[[nodiscard]] constexpr float LifeLeft(const Particle& particle) {
    if (particle.seconds <= 0.0f) {
        return 0.0f;
    }
    const float share = particle.secondsLeft / particle.seconds;
    return share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
}

/// Moves a particle on by `dt` seconds: its life, its drag, its flight and fall, its bounces, its spin and
/// its growth. A particle lying on the floor also slides to a stop.
///
/// @return Whether it is still alive.
bool StepParticle(Particle& particle, float dt);

/// Steps every particle in the pool, and removes the ones whose life ran out.
void StepParticles(ParticlePool& particles, float dt);

/// The same, bouncing particles off the walls of `arena` so casings and shards stay inside the room.
void StepParticles(ParticlePool& particles, const Arena& arena, float dt);

} // namespace horde
