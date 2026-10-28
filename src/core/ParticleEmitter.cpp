#include "core/ParticleEmitter.h"

#include <cmath>
#include <numbers>

#include "core/Boss.h"
#include "core/Dash.h"
#include "core/Health.h"
#include "core/ImpactFeel.h"
#include "core/Shield.h"

namespace horde {

namespace {

constexpr std::uint64_t kParticleStream = 11;
constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;

// Matches the shield band drawn outside the boss's tufts (`radius * 1.5 + 6`), so shards start on the ring.
constexpr float kShieldTuftScale = 1.5f;
constexpr float kShieldRingGap = 6.0f;

// Continuous emission intervals, in seconds between spawns.
constexpr float kDashDustInterval = 0.02f;
constexpr float kPhaseTwoHairInterval = 1.0f / 9.0f;
constexpr float kTransformHairInterval = 1.0f / 15.0f;
constexpr float kFrenzyHairInterval = 1.0f / 22.0f;

Vec2 Polar(float length, float radians) {
    return { std::cos(radians) * length, std::sin(radians) * length };
}

float RandomAngle(Random& random) {
    return RandomRange(random, 0.0f, kTwoPi);
}

float RandomSign(Random& random) {
    return Chance(random, 0.5f) ? 1.0f : -1.0f;
}

Vec2 SafeDirection(Vec2 direction, Vec2 fallback = {0.0f, -1.0f}) {
    return LengthSquared(direction) > 0.0f ? Normalized(direction) : fallback;
}

ParticleTint ShieldTint(ShieldKind kind) {
    return kind == ShieldKind::Shell ? ParticleTint::Shell : ParticleTint::Barrier;
}

void SpawnSpark(ParticleEmitter& emitter, ParticleTint tint, Vec2 position, Vec2 direction, float minSpeed,
                float maxSpeed, float minLife, float maxLife) {
    Random& rng = emitter.random;
    const float speed = RandomRange(rng, minSpeed, maxSpeed);
    emitter.particles.Spawn(WithLife(
        Particle{
            .kind = ParticleKind::Spark,
            .tint = tint,
            .position = position,
            .velocity = direction * speed,
            .drag = 8.5f,
            .angle = std::atan2(direction.y, direction.x),
            .size = RandomRange(rng, 5.0f, 9.5f),
        },
        RandomRange(rng, minLife, maxLife)));
}

void SpawnDust(ParticleEmitter& emitter, Vec2 position, Vec2 velocity, float minSize, float maxSize,
               float growth, float minLife, float maxLife) {
    Random& rng = emitter.random;
    emitter.particles.Spawn(WithLife(
        Particle{
            .kind = ParticleKind::Dust,
            .tint = ParticleTint::Plain,
            .position = position,
            .velocity = velocity,
            .drag = 4.2f,
            .height = RandomRange(rng, 1.0f, 6.0f),
            .lift = RandomRange(rng, 8.0f, 24.0f),
            .size = RandomRange(rng, minSize, maxSize),
            .growth = growth,
        },
        RandomRange(rng, minLife, maxLife)));
}

void SpawnBlood(ParticleEmitter& emitter, ParticleTint tint, Vec2 position, Vec2 velocity, float minSize,
                float maxSize, float minLife, float maxLife) {
    Random& rng = emitter.random;
    emitter.particles.Spawn(WithLife(
        Particle{
            .kind = ParticleKind::Blood,
            .tint = tint,
            .position = position,
            .velocity = velocity,
            .drag = 4.2f,
            .height = RandomRange(rng, 8.0f, 20.0f),
            .lift = RandomRange(rng, 35.0f, 110.0f),
            .gravity = 560.0f,
            .bounce = 0.0f,
            .angle = RandomAngle(rng),
            .size = RandomRange(rng, minSize, maxSize),
        },
        RandomRange(rng, minLife, maxLife)));
}

void SpawnHairLock(ParticleEmitter& emitter, ParticleTint tint, Vec2 position, Vec2 velocity, float height,
                   float lift, float gravity, float minLife, float maxLife) {
    Random& rng = emitter.random;
    emitter.particles.Spawn(WithLife(
        Particle{
            .kind = ParticleKind::Hair,
            .tint = tint,
            .position = position,
            .velocity = velocity,
            .drag = 4.6f,
            .height = height,
            .lift = lift,
            .gravity = gravity,
            .bounce = 0.15f,
            .angle = RandomAngle(rng),
            .spin = RandomSign(rng) * RandomRange(rng, 5.0f, 14.0f),
            .size = RandomRange(rng, 7.0f, 12.5f),
        },
        RandomRange(rng, minLife, maxLife)));
}

void SpawnShard(ParticleEmitter& emitter, ParticleTint tint, Vec2 position, Vec2 velocity, float minSize,
                float maxSize, float minLife, float maxLife) {
    Random& rng = emitter.random;
    emitter.particles.Spawn(WithLife(
        Particle{
            .kind = ParticleKind::Shard,
            .tint = tint,
            .position = position,
            .velocity = velocity,
            .drag = 3.2f,
            .height = RandomRange(rng, 10.0f, 26.0f),
            .lift = RandomRange(rng, 50.0f, 145.0f),
            .gravity = 520.0f,
            .bounce = 0.4f,
            .angle = RandomAngle(rng),
            .spin = RandomSign(rng) * RandomRange(rng, 8.0f, 17.0f),
            .size = RandomRange(rng, minSize, maxSize),
        },
        RandomRange(rng, minLife, maxLife)));
}

void EmitShot(ParticleEmitter& emitter, const WorldEvent& event, const World& world) {
    Random& rng = emitter.random;
    const Vec2 forward = SafeDirection(event.direction);
    const Vec2 right{-forward.y, forward.x};
    const bool heavyGun = event.amount >= 9;

    const int sparks = heavyGun ? 5 : 3;
    for (int i = 0; i < sparks; ++i) {
        const Vec2 aim = Rotated(forward, RandomRange(rng, -0.42f, 0.42f));
        SpawnSpark(emitter, ParticleTint::Plain, event.position, aim, heavyGun ? 280.0f : 230.0f,
                   heavyGun ? 540.0f : 440.0f, 0.08f, 0.16f);
    }

    const int puffs = heavyGun ? 2 : 1;
    for (int i = 0; i < puffs; ++i) {
        const Vec2 drift = Rotated(forward, RandomRange(rng, -0.55f, 0.55f)) * RandomRange(rng, 35.0f, 80.0f);
        emitter.particles.Spawn(WithLife(
            Particle{
                .kind = ParticleKind::Smoke,
                .tint = ParticleTint::Plain,
                .position = event.position + forward * 4.0f,
                .velocity = drift,
                .drag = 4.5f,
                .height = 6.0f,
                .lift = RandomRange(rng, 10.0f, 24.0f),
                .size = RandomRange(rng, 4.0f, 6.5f),
                .growth = RandomRange(rng, 14.0f, 22.0f),
            },
            RandomRange(rng, 0.28f, 0.45f)));
    }

    // Ejected from the breech behind the muzzle, out to the hunter's right and slightly back.
    const Vec2 breech = event.position - forward * 12.0f + right * 4.0f;
    const Vec2 eject = Rotated(Normalized(right - forward * 0.3f), RandomRange(rng, -0.35f, 0.35f));
    emitter.particles.Spawn(WithLife(
        Particle{
            .kind = ParticleKind::Casing,
            .tint = ParticleTint::Plain,
            .position = breech,
            .velocity = eject * RandomRange(rng, 85.0f, 145.0f) + world.player.motion.velocity * 0.35f,
            .drag = 3.2f,
            .height = 14.0f,
            .lift = RandomRange(rng, 65.0f, 115.0f),
            .gravity = 480.0f,
            .bounce = 0.45f,
            .angle = RandomAngle(rng),
            .spin = RandomSign(rng) * RandomRange(rng, 12.0f, 22.0f),
            .size = heavyGun ? 5.2f : 4.2f,
        },
        RandomRange(rng, 4.0f, 5.5f)));
}

void EmitDashStart(ParticleEmitter& emitter, const WorldEvent& event) {
    Random& rng = emitter.random;
    const Vec2 forward = SafeDirection(event.direction);
    const Vec2 back = forward * -1.0f;
    const Vec2 side{-forward.y, forward.x};
    for (int i = 0; i < 10; ++i) {
        const Vec2 at = event.position + side * RandomRange(rng, -10.0f, 10.0f);
        const Vec2 velocity = Rotated(back, RandomRange(rng, -0.9f, 0.9f)) * RandomRange(rng, 45.0f, 140.0f);
        SpawnDust(emitter, at, velocity, 5.5f, 9.5f, RandomRange(rng, 16.0f, 26.0f), 0.28f, 0.48f);
    }
}

void EmitShieldStrike(ParticleEmitter& emitter, const WorldEvent& event, const World& world) {
    Random& rng = emitter.random;
    const ParticleTint tint = ShieldTint(world.boss.shield.kind);
    const Vec2 incoming = SafeDirection(event.direction, {0.0f, -1.0f});
    const Vec2 outward = SafeDirection(event.position - world.boss.motion.position, incoming * -1.0f);

    if (event.amount <= 0) {
        // Bounced off the Shell: a tight specular ricochet, with no chips taken off the ring.
        const float into = Dot(incoming, outward);
        const Vec2 bounceDir =
            into < 0.0f ? Normalized(incoming - outward * (2.0f * into)) : outward;
        for (int i = 0; i < 8; ++i) {
            const Vec2 aim = Rotated(bounceDir, RandomRange(rng, -0.55f, 0.55f));
            SpawnSpark(emitter, tint, event.position, aim, 280.0f, 560.0f, 0.12f, 0.24f);
        }
        return;
    }

    for (int i = 0; i < 10; ++i) {
        const Vec2 aim = Rotated(outward, RandomRange(rng, -1.05f, 1.05f));
        SpawnSpark(emitter, tint, event.position, aim, 200.0f, 440.0f, 0.14f, 0.28f);
    }
    for (int i = 0; i < 2; ++i) {
        const Vec2 chipVel = Rotated(outward, RandomRange(rng, -0.8f, 0.8f)) * RandomRange(rng, 70.0f, 150.0f);
        SpawnShard(emitter, tint, event.position, chipVel, 3.5f, 5.5f, 0.8f, 1.4f);
    }
}

void EmitShieldBreak(ParticleEmitter& emitter, const WorldEvent& event, const World& world) {
    Random& rng = emitter.random;
    const ParticleTint tint = ShieldTint(world.boss.shield.kind);
    const Vec2 center = world.boss.motion.position;
    const float ringRadius = world.boss.radius * kShieldTuftScale + kShieldRingGap;
    const Vec2 push = event.direction * 45.0f;

    constexpr int kShards = 28;
    for (int i = 0; i < kShards; ++i) {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(kShards) +
                            RandomRange(rng, -0.08f, 0.08f);
        const Vec2 out = Polar(1.0f, angle);
        const Vec2 at = center + out * ringRadius;
        const Vec2 velocity = out * RandomRange(rng, 140.0f, 340.0f) + push;
        SpawnShard(emitter, tint, at, velocity, 5.5f, 10.5f, 2.2f, 3.6f);
    }

    constexpr int kSparks = 18;
    for (int i = 0; i < kSparks; ++i) {
        const float angle = RandomAngle(rng);
        const Vec2 out = Polar(1.0f, angle);
        SpawnSpark(emitter, tint, center + out * ringRadius, out, 220.0f, 520.0f, 0.18f, 0.36f);
    }
}

void EmitBossHurt(ParticleEmitter& emitter, const WorldEvent& event, const World& world) {
    Random& rng = emitter.random;
    const Vec2 incoming = SafeDirection(event.direction);
    const Vec2 outward = SafeDirection(event.position - world.boss.motion.position, incoming * -1.0f);
    const Vec2 spray = SafeDirection(outward + incoming * 0.35f, outward);
    const bool heavyHit = event.amount >= 9;

    const int drops = heavyHit ? 6 : 4;
    for (int i = 0; i < drops; ++i) {
        const Vec2 velocity = Rotated(spray, RandomRange(rng, -0.95f, 0.95f)) * RandomRange(rng, 70.0f, 210.0f);
        SpawnBlood(emitter, ParticleTint::Boss, event.position, velocity, 3.2f, 6.2f, 2.8f, 4.5f);
    }

    const int locks = heavyHit ? 5 : 3;
    for (int i = 0; i < locks; ++i) {
        const Vec2 velocity = Rotated(spray, RandomRange(rng, -1.1f, 1.1f)) * RandomRange(rng, 55.0f, 175.0f);
        SpawnHairLock(emitter, ParticleTint::Boss, event.position, velocity, RandomRange(rng, 10.0f, 24.0f),
                      RandomRange(rng, 30.0f, 95.0f), 320.0f, 3.0f, 4.8f);
    }
}

void EmitHairballKill(ParticleEmitter& emitter, const WorldEvent& event) {
    Random& rng = emitter.random;
    const Vec2 carry = event.direction * 55.0f;

    constexpr int kLocks = 18;
    for (int i = 0; i < kLocks; ++i) {
        const Vec2 out = Polar(RandomRange(rng, 60.0f, 210.0f), RandomAngle(rng));
        const Vec2 at = event.position + Polar(RandomRange(rng, 0.0f, 12.0f), RandomAngle(rng));
        SpawnHairLock(emitter, ParticleTint::Hairball, at, out + carry, RandomRange(rng, 6.0f, 18.0f),
                      RandomRange(rng, 40.0f, 120.0f), 340.0f, 2.4f, 4.0f);
    }
    for (int i = 0; i < 6; ++i) {
        const Vec2 out = Polar(RandomRange(rng, 50.0f, 160.0f), RandomAngle(rng));
        SpawnBlood(emitter, ParticleTint::Hairball, event.position, out + carry * 0.6f, 2.8f, 5.0f, 2.2f, 3.6f);
    }
    for (int i = 0; i < 5; ++i) {
        const Vec2 out = Polar(RandomRange(rng, 40.0f, 110.0f), RandomAngle(rng));
        SpawnDust(emitter, event.position, out, 6.0f, 10.0f, RandomRange(rng, 16.0f, 24.0f), 0.30f, 0.50f);
    }
}

void EmitBossLanding(ParticleEmitter& emitter, const WorldEvent& event, const World& world) {
    Random& rng = emitter.random;
    constexpr int kRingPuffs = 28;
    for (int i = 0; i < kRingPuffs; ++i) {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(kRingPuffs) +
                            RandomRange(rng, -0.06f, 0.06f);
        const Vec2 out = Polar(1.0f, angle);
        const Vec2 at = event.position + out * (world.boss.radius * RandomRange(rng, 1.05f, 1.30f));
        SpawnDust(emitter, at, out * RandomRange(rng, 250.0f, 360.0f), 10.0f, 15.5f,
                  RandomRange(rng, 24.0f, 38.0f), 0.48f, 0.76f);
    }
    for (int i = 0; i < 12; ++i) {
        const Vec2 out = Polar(1.0f, RandomAngle(rng));
        SpawnSpark(emitter, ParticleTint::Plain, event.position + out * (world.boss.radius * 1.15f), out,
                   220.0f, 440.0f, 0.18f, 0.30f);
    }
}

void EmitBossWallCrash(ParticleEmitter& emitter, const WorldEvent& event, const World& world) {
    Random& rng = emitter.random;
    const Vec2 into = SafeDirection(event.direction, {1.0f, 0.0f});
    const Vec2 back = into * -1.0f;
    const Vec2 contact = event.position + into * world.boss.radius;

    for (int i = 0; i < 14; ++i) {
        const Vec2 velocity = Rotated(back, RandomRange(rng, -1.15f, 1.15f)) * RandomRange(rng, 80.0f, 240.0f);
        SpawnDust(emitter, contact, velocity, 8.0f, 13.0f, RandomRange(rng, 20.0f, 32.0f), 0.40f, 0.68f);
    }
    for (int i = 0; i < 10; ++i) {
        const Vec2 aim = Rotated(back, RandomRange(rng, -1.0f, 1.0f));
        SpawnSpark(emitter, ParticleTint::Plain, contact, aim, 200.0f, 460.0f, 0.15f, 0.28f);
    }
    for (int i = 0; i < 8; ++i) {
        const Vec2 velocity = Rotated(back, RandomRange(rng, -1.2f, 1.2f)) * RandomRange(rng, 60.0f, 190.0f);
        SpawnHairLock(emitter, ParticleTint::Boss, event.position, velocity, RandomRange(rng, 12.0f, 28.0f),
                      RandomRange(rng, 40.0f, 110.0f), 320.0f, 2.6f, 4.2f);
    }
}

void EmitPlayerBlood(ParticleEmitter& emitter, Vec2 position, int drops) {
    Random& rng = emitter.random;
    for (int i = 0; i < drops; ++i) {
        const Vec2 velocity = Polar(RandomRange(rng, 55.0f, 195.0f), RandomAngle(rng));
        SpawnBlood(emitter, ParticleTint::Plain, position, velocity, 3.0f, 5.8f, 3.0f, 4.8f);
    }
}

void EmitBossBurst(ParticleEmitter& emitter, Vec2 center, float radius, int locks, int bloodDrops, int dustPuffs) {
    Random& rng = emitter.random;
    for (int i = 0; i < locks; ++i) {
        const float angle = RandomAngle(rng);
        const Vec2 out = Polar(1.0f, angle);
        const Vec2 at = center + out * (radius * RandomRange(rng, 0.2f, 0.9f));
        SpawnHairLock(emitter, ParticleTint::Boss, at, out * RandomRange(rng, 80.0f, 260.0f),
                      RandomRange(rng, 12.0f, 34.0f), RandomRange(rng, 50.0f, 150.0f), 300.0f, 3.2f, 5.2f);
    }
    for (int i = 0; i < bloodDrops; ++i) {
        const Vec2 out = Polar(RandomRange(rng, 60.0f, 230.0f), RandomAngle(rng));
        SpawnBlood(emitter, ParticleTint::Boss, center, out, 3.5f, 6.8f, 3.2f, 5.0f);
    }
    for (int i = 0; i < dustPuffs; ++i) {
        const float angle = RandomAngle(rng);
        const Vec2 out = Polar(1.0f, angle);
        SpawnDust(emitter, center + out * (radius * 0.5f), out * RandomRange(rng, 90.0f, 210.0f), 8.0f, 13.0f,
                  RandomRange(rng, 20.0f, 30.0f), 0.40f, 0.68f);
    }
}

} // namespace

ParticleEmitter SeedParticleEmitter(std::uint64_t seed) {
    return ParticleEmitter{.random = SeedRandom(seed, kParticleStream)};
}

void ClearParticles(ParticleEmitter& emitter) {
    emitter.particles.Clear();
    emitter.dashDustSeconds = 0.0f;
    emitter.bossHairSeconds = 0.0f;
}

void EmitEventParticles(ParticleEmitter& emitter, const WorldEvent& event, const World& world) {
    switch (event.kind) {
    case WorldEventKind::ShotFired:
        EmitShot(emitter, event, world);
        break;
    case WorldEventKind::DashStarted:
        EmitDashStart(emitter, event);
        break;
    case WorldEventKind::PlayerHurt:
        EmitPlayerBlood(emitter, event.position, event.amount >= kHeavyHitDamage ? 14 : 8);
        break;
    case WorldEventKind::PlayerKilled:
        EmitPlayerBlood(emitter, event.position, 22);
        break;
    case WorldEventKind::BossHurt:
        EmitBossHurt(emitter, event, world);
        break;
    case WorldEventKind::ShieldStruck:
        EmitShieldStrike(emitter, event, world);
        break;
    case WorldEventKind::ShieldBroken:
        EmitShieldBreak(emitter, event, world);
        break;
    case WorldEventKind::HairballKilled:
        EmitHairballKill(emitter, event);
        break;
    case WorldEventKind::BossLanded:
        EmitBossLanding(emitter, event, world);
        break;
    case WorldEventKind::BossHitWall:
        EmitBossWallCrash(emitter, event, world);
        break;
    case WorldEventKind::FirstPhaseEnded:
    case WorldEventKind::BossRoared:
        EmitBossBurst(emitter, event.position, world.boss.radius, 18, 0, 12);
        break;
    case WorldEventKind::BossDefeated:
        EmitBossBurst(emitter, event.position, world.boss.radius, 36, 20, 16);
        break;
    // Heard, not seen: the warnings, the spitting and the drink already show on the boss and the player.
    case WorldEventKind::AttackStarted:
    case WorldEventKind::StrandsSpat:
    case WorldEventKind::FlaskDrunk:
        break;
    }
}

void EmitContinuousParticles(ParticleEmitter& emitter, const World& world, float dt) {
    if (dt <= 0.0f) {
        return;
    }
    Random& rng = emitter.random;

    const Player& player = world.player;
    if (IsDashing(player.dash)) {
        emitter.dashDustSeconds += dt;
        const Vec2 back = SafeDirection(player.dash.direction) * -1.0f;
        const Vec2 side{-back.y, back.x};
        while (emitter.dashDustSeconds >= kDashDustInterval) {
            emitter.dashDustSeconds -= kDashDustInterval;
            for (int i = 0; i < 2; ++i) {
                const Vec2 at = player.motion.position + side * RandomRange(rng, -9.0f, 9.0f);
                const Vec2 drift =
                    Rotated(back, RandomRange(rng, -0.7f, 0.7f)) * RandomRange(rng, 25.0f, 70.0f);
                SpawnDust(emitter, at, drift, 4.5f, 7.5f, RandomRange(rng, 14.0f, 22.0f), 0.24f, 0.40f);
            }
        }
    } else {
        emitter.dashDustSeconds = 0.0f;
    }

    const Boss& boss = world.boss;
    const bool shedsHair = !IsDead(boss.health) && !IsAirborne(boss) && boss.phase != BossPhase::One;
    if (!shedsHair) {
        emitter.bossHairSeconds = 0.0f;
        return;
    }

    const float interval = boss.frenzied             ? kFrenzyHairInterval
                           : IsTransforming(boss)    ? kTransformHairInterval
                                                     : kPhaseTwoHairInterval;
    emitter.bossHairSeconds += dt;
    while (emitter.bossHairSeconds >= interval) {
        emitter.bossHairSeconds -= interval;
        const float angle = RandomAngle(rng);
        const Vec2 out = Polar(1.0f, angle);
        const Vec2 at = boss.motion.position + out * (boss.radius * RandomRange(rng, 0.35f, 0.98f));
        const Vec2 velocity = boss.motion.velocity * 0.25f + out * RandomRange(rng, 14.0f, 48.0f);
        SpawnHairLock(emitter, ParticleTint::Boss, at, velocity, RandomRange(rng, 14.0f, 34.0f),
                      RandomRange(rng, -10.0f, 25.0f), 260.0f, 2.6f, 4.2f);
    }
}

} // namespace horde
