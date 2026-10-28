#include "core/Particle.h"

#include <algorithm>
#include <cmath>

namespace horde {

namespace {

// A bounce weaker than this is less than half a unit high and cannot be seen, so the particle settles on
// the floor instead of chattering.
constexpr float kMinBounceLift = 20.0f;
// Once a falling particle lies on the floor, stone friction stops any leftover slide quickly.
constexpr float kFloorDrag = 14.0f;
// Share of speed kept when a particle ricochets off a crypt wall.
constexpr float kWallBounce = 0.4f;

void ConfineParticle(Particle& particle, const Arena& arena) {
    const float margin = std::max(particle.size * 0.5f, 1.0f);
    const Vec2 low = arena.min + Vec2{margin, margin};
    const Vec2 high = arena.max - Vec2{margin, margin};
    if (particle.position.x < low.x) {
        particle.position.x = low.x;
        particle.velocity.x = std::abs(particle.velocity.x) * kWallBounce;
    } else if (particle.position.x > high.x) {
        particle.position.x = high.x;
        particle.velocity.x = -std::abs(particle.velocity.x) * kWallBounce;
    }
    if (particle.position.y < low.y) {
        particle.position.y = low.y;
        particle.velocity.y = std::abs(particle.velocity.y) * kWallBounce;
    } else if (particle.position.y > high.y) {
        particle.position.y = high.y;
        particle.velocity.y = -std::abs(particle.velocity.y) * kWallBounce;
    }
}

} // namespace

bool StepParticle(Particle& particle, float dt) {
    if (dt <= 0.0f) {
        return particle.secondsLeft > 0.0f;
    }
    particle.secondsLeft -= dt;
    if (particle.secondsLeft <= 0.0f) {
        particle.secondsLeft = 0.0f;
        return false;
    }

    const bool settled = IsOnFloor(particle);
    const float drag = std::max(particle.drag, 0.0f) + (settled ? kFloorDrag : 0.0f);
    // Exact integration of exponential drag, so two half-steps land in the same place as one full step
    // whatever the monitor's refresh rate, and a long frame can never reverse the velocity.
    if (drag > 0.0f) {
        const float decay = std::exp(-drag * dt);
        particle.position += particle.velocity * ((1.0f - decay) / drag);
        particle.velocity *= decay;
    } else {
        particle.position += particle.velocity * dt;
    }

    if (settled) {
        return true;
    }

    particle.angle += particle.spin * dt;
    particle.size = std::max(particle.size + particle.growth * dt, 0.0f);

    if (particle.gravity <= 0.0f) {
        particle.height = std::max(particle.height + particle.lift * dt, 0.0f);
        return true;
    }

    particle.height += particle.lift * dt - 0.5f * particle.gravity * dt * dt;
    particle.lift -= particle.gravity * dt;
    if (particle.height <= 0.0f) {
        particle.height = 0.0f;
        const float rebound = -particle.lift * std::clamp(particle.bounce, 0.0f, 1.0f);
        if (rebound >= kMinBounceLift) {
            particle.lift = rebound;
            particle.velocity *= particle.bounce;
            particle.spin *= particle.bounce;
        } else {
            particle.lift = 0.0f;
            particle.spin = 0.0f;
        }
    }
    return true;
}

void StepParticles(ParticlePool& particles, float dt) {
    particles.RemoveIf([dt](Particle& particle) { return !StepParticle(particle, dt); });
}

void StepParticles(ParticlePool& particles, const Arena& arena, float dt) {
    particles.RemoveIf([&arena, dt](Particle& particle) {
        if (!StepParticle(particle, dt)) {
            return true;
        }
        ConfineParticle(particle, arena);
        return false;
    });
}

} // namespace horde
