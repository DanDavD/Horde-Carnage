#include <algorithm>
#include <cstddef>

#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/ParticleEmitter.h"
#include "core/World.h"

using horde::IsOnFloor;
using horde::Particle;
using horde::ParticleEmitter;
using horde::ParticleKind;
using horde::ParticleTint;
using horde::PlayerCommands;
using horde::SeedParticleEmitter;
using horde::Vec2;
using horde::World;
using horde::WorldEvent;
using horde::WorldEventKind;

namespace {

constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

std::size_t CountKind(const ParticleEmitter& emitter, ParticleKind kind) {
    return static_cast<std::size_t>(std::count_if(
        emitter.particles.begin(), emitter.particles.end(),
        [kind](const Particle& particle) { return particle.kind == kind; }));
}

std::size_t CountKindTint(const ParticleEmitter& emitter, ParticleKind kind, ParticleTint tint) {
    return static_cast<std::size_t>(std::count_if(
        emitter.particles.begin(), emitter.particles.end(),
        [kind, tint](const Particle& particle) { return particle.kind == kind && particle.tint == tint; }));
}

} // namespace

TEST_CASE("emitting and stepping particles leaves the fight's random state and outcome untouched") {
    World plain = horde::CreateWorld(horde::kBossArena, kView, 0xC0FFEEULL);
    World juicy = horde::CreateWorld(horde::kBossArena, kView, 0xC0FFEEULL);
    ParticleEmitter emitter = SeedParticleEmitter(42);

    for (int step = 0; step < 360; ++step) {
        const PlayerCommands commands{
            .move = step % 90 < 45 ? Vec2{1.0f, 0.0f} : Vec2{-1.0f, 0.2f},
            .aimAt = plain.boss.motion.position,
            .fireHeld = true,
            .firePressed = (step % 12 == 0),
            .cycleWeapon = (step == 120 || step == 240),
            .dashPressed = (step % 50 == 10),
        };

        horde::StepWorld(plain, commands, kDt);
        plain.events.Clear();

        horde::StepWorld(juicy, commands, kDt);
        horde::EmitContinuousParticles(emitter, juicy, kDt);
        for (const WorldEvent& event : juicy.events) {
            horde::EmitEventParticles(emitter, event, juicy);
        }
        juicy.events.Clear();
        horde::StepParticles(emitter.particles, juicy.arena, kDt);
    }

    CHECK_FALSE(emitter.particles.Empty());
    CHECK(juicy.random.state == plain.random.state);
    CHECK(juicy.random.increment == plain.random.increment);
    CHECK(juicy.player.motion.position == plain.player.motion.position);
    CHECK(juicy.boss.motion.position == plain.boss.motion.position);
    CHECK(juicy.boss.health.current == plain.boss.health.current);
    CHECK(juicy.boss.shield.strength == doctest::Approx(plain.boss.shield.strength));
    CHECK(juicy.shotsFired == plain.shotsFired);
    CHECK(juicy.shotsHit == plain.shotsHit);
    CHECK(juicy.enemies.Size() == plain.enemies.Size());
}

TEST_CASE("every shot throws muzzle sparks, gunsmoke and a brass casing that lands and stays on the floor") {
    const World world = horde::CreateWorld(horde::kBossArena, kView);
    ParticleEmitter emitter = SeedParticleEmitter(7);

    horde::EmitEventParticles(
        emitter,
        WorldEvent{.kind = WorldEventKind::ShotFired,
                   .position = {820.0f, 900.0f},
                   .direction = {0.0f, -1.0f},
                   .amount = horde::kPistol.damage},
        world);

    CHECK(CountKind(emitter, ParticleKind::Spark) >= 3);
    CHECK(CountKind(emitter, ParticleKind::Smoke) >= 1);
    REQUIRE(CountKind(emitter, ParticleKind::Casing) == 1);

    const auto casingIt = std::find_if(emitter.particles.begin(), emitter.particles.end(),
                                       [](const Particle& p) { return p.kind == ParticleKind::Casing; });
    REQUIRE(casingIt != emitter.particles.end());
    // Looking north (0, -1), the hunter's right is +x, so the casing ejects to the east.
    CHECK(casingIt->velocity.x > 0.0f);

    // Step past the bounces (~1.2 s): sparks and smoke are gone, and the casing lies still on the stone.
    for (int i = 0; i < 72; ++i) {
        horde::StepParticles(emitter.particles, world.arena, kDt);
    }
    CHECK(CountKind(emitter, ParticleKind::Spark) == 0);
    CHECK(CountKind(emitter, ParticleKind::Smoke) == 0);
    REQUIRE(CountKind(emitter, ParticleKind::Casing) == 1);
    CHECK(IsOnFloor(emitter.particles[0]));
}

TEST_CASE("shield strikes and breaks take the blue Shell or orange Barrier tint") {
    World world = horde::CreateWorld(horde::kBossArena, kView);
    ParticleEmitter emitter = SeedParticleEmitter(9);

    // Bounced SMG hit on the blue Shell: blue sparks, no chipped shards.
    world.boss.shield.kind = horde::ShieldKind::Shell;
    horde::EmitEventParticles(
        emitter,
        WorldEvent{.kind = WorldEventKind::ShieldStruck,
                   .position = world.boss.motion.position + Vec2{0.0f, 60.0f},
                   .direction = {0.0f, -1.0f},
                   .amount = 0},
        world);
    CHECK(CountKindTint(emitter, ParticleKind::Spark, ParticleTint::Shell) > 0);
    CHECK(CountKind(emitter, ParticleKind::Shard) == 0);

    // Absorbed hit on the orange Barrier: orange sparks and small orange chips.
    horde::ClearParticles(emitter);
    world.boss.shield.kind = horde::ShieldKind::Barrier;
    horde::EmitEventParticles(
        emitter,
        WorldEvent{.kind = WorldEventKind::ShieldStruck,
                   .position = world.boss.motion.position + Vec2{0.0f, 60.0f},
                   .direction = {0.0f, -1.0f},
                   .amount = 6},
        world);
    CHECK(CountKindTint(emitter, ParticleKind::Spark, ParticleTint::Barrier) > 0);
    CHECK(CountKindTint(emitter, ParticleKind::Shard, ParticleTint::Barrier) > 0);

    // Breaking the shield shatters a full ring of shards in the broken shield's colour.
    horde::ClearParticles(emitter);
    horde::BreakBossShield(world);
    REQUIRE_FALSE(world.events.Empty());
    horde::EmitEventParticles(emitter, world.events[0], world);
    CHECK(CountKindTint(emitter, ParticleKind::Shard, ParticleTint::Barrier) >= 20);
}

TEST_CASE("bullets on the exposed boss throw dark blood and locks of her hair") {
    const World world = horde::CreateWorld(horde::kBossArena, kView);
    ParticleEmitter emitter = SeedParticleEmitter(13);

    horde::EmitEventParticles(
        emitter,
        WorldEvent{.kind = WorldEventKind::BossHurt,
                   .position = world.boss.motion.position + Vec2{0.0f, 55.0f},
                   .direction = {0.0f, -1.0f},
                   .amount = horde::kPistol.damage},
        world);

    CHECK(CountKindTint(emitter, ParticleKind::Blood, ParticleTint::Boss) > 0);
    CHECK(CountKindTint(emitter, ParticleKind::Hair, ParticleTint::Boss) > 0);
}

TEST_CASE("a dash, a leap landing and a killed hairball each throw their own burst") {
    World world = horde::CreateWorld(horde::kBossArena, kView);
    world.boss = horde::test::Predictable(world.boss);
    ParticleEmitter emitter = SeedParticleEmitter(21);

    // Starting a dash kicks dust backward, and dashing continues to leave dust along the run.
    horde::StepWorld(world, PlayerCommands{.move = {1.0f, 0.0f}, .dashPressed = true}, kDt);
    for (const WorldEvent& event : world.events) {
        horde::EmitEventParticles(emitter, event, world);
    }
    const std::size_t takeoffDust = CountKind(emitter, ParticleKind::Dust);
    CHECK(takeoffDust > 0);
    horde::EmitContinuousParticles(emitter, world, 0.05f);
    CHECK(CountKind(emitter, ParticleKind::Dust) > takeoffDust);

    // A leap landing sends a ring of dust outward in every quadrant around the impact.
    horde::ClearParticles(emitter);
    const Vec2 landingSpot{820.0f, 520.0f};
    horde::EmitEventParticles(
        emitter, WorldEvent{.kind = WorldEventKind::BossLanded, .position = landingSpot}, world);
    bool east = false;
    bool west = false;
    bool north = false;
    bool south = false;
    for (const Particle& p : emitter.particles) {
        if (p.kind == ParticleKind::Dust) {
            east = east || p.velocity.x > 80.0f;
            west = west || p.velocity.x < -80.0f;
            south = south || p.velocity.y > 80.0f;
            north = north || p.velocity.y < -80.0f;
        }
    }
    CHECK(east);
    CHECK(west);
    CHECK(north);
    CHECK(south);

    // Killing a hairball bursts lighter hairball locks.
    horde::ClearParticles(emitter);
    horde::EmitEventParticles(
        emitter,
        WorldEvent{.kind = WorldEventKind::HairballKilled, .position = {700.0f, 600.0f}, .direction = {0.0f, -1.0f}},
        world);
    CHECK(CountKindTint(emitter, ParticleKind::Hair, ParticleTint::Hairball) >= 12);
}

TEST_CASE("the boss sheds no hair in phase 1, sheds in phase 2 and sheds faster in frenzy") {
    World world = horde::CreateWorld(horde::kBossArena, kView);
    ParticleEmitter emitter = SeedParticleEmitter(33);

    horde::EmitContinuousParticles(emitter, world, 1.0f);
    CHECK(CountKind(emitter, ParticleKind::Hair) == 0);

    world.boss.phase = horde::BossPhase::Two;
    horde::EmitContinuousParticles(emitter, world, 1.0f);
    const std::size_t phaseTwoLocks = CountKindTint(emitter, ParticleKind::Hair, ParticleTint::Boss);
    CHECK(phaseTwoLocks >= 8);

    horde::ClearParticles(emitter);
    world.boss.frenzied = true;
    horde::EmitContinuousParticles(emitter, world, 1.0f);
    const std::size_t frenzyLocks = CountKindTint(emitter, ParticleKind::Hair, ParticleTint::Boss);
    CHECK(frenzyLocks > phaseTwoLocks * 2);

    // A dead boss stops shedding.
    horde::ClearParticles(emitter);
    world.boss.health.current = 0;
    horde::EmitContinuousParticles(emitter, world, 1.0f);
    CHECK(emitter.particles.Empty());
}

TEST_CASE("emitting into an already-full pool caps cleanly at kMaxParticles") {
    const World world = horde::CreateWorld(horde::kBossArena, kView);
    ParticleEmitter emitter = SeedParticleEmitter(55);

    for (int i = 0; i < 40; ++i) {
        horde::EmitEventParticles(
            emitter, WorldEvent{.kind = WorldEventKind::BossDefeated, .position = world.boss.motion.position},
            world);
    }
    CHECK(emitter.particles.Size() == emitter.particles.MaxSize());
    CHECK(emitter.particles.Size() == horde::kMaxParticles);

    horde::ClearParticles(emitter);
    CHECK(emitter.particles.Empty());
}
