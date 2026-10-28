#include <doctest/doctest.h>

#include "core/Arena.h"
#include "core/Particle.h"

using horde::Arena;
using horde::IsOnFloor;
using horde::LifeLeft;
using horde::Particle;
using horde::ParticleKind;
using horde::ParticlePool;
using horde::StepParticle;
using horde::StepParticles;
using horde::Vec2;
using horde::WithLife;

TEST_CASE("LifeLeft runs from 1 down to 0 and clamps out-of-range values") {
    Particle p = WithLife(Particle{}, 2.0f);
    CHECK(LifeLeft(p) == doctest::Approx(1.0f));

    p.secondsLeft = 0.5f;
    CHECK(LifeLeft(p) == doctest::Approx(0.25f));

    p.secondsLeft = -0.2f;
    CHECK(LifeLeft(p) == 0.0f);

    p.secondsLeft = 3.0f;
    CHECK(LifeLeft(p) == 1.0f);

    p.seconds = 0.0f;
    CHECK(LifeLeft(p) == 0.0f);
}

TEST_CASE("StepParticle counts life down and StepParticles removes only expired ones") {
    ParticlePool pool;
    pool.Spawn(WithLife(Particle{.position = {10.0f, 0.0f}}, 0.2f));
    pool.Spawn(WithLife(Particle{.position = {20.0f, 0.0f}}, 1.0f));

    StepParticles(pool, 0.0f);
    CHECK(pool.Size() == 2);

    StepParticles(pool, 0.25f);
    REQUIRE(pool.Size() == 1);
    CHECK(pool[0].position.x == doctest::Approx(20.0f));
    CHECK(pool[0].secondsLeft == doctest::Approx(0.75f));
}

TEST_CASE("exponential drag slows a particle smoothly, never reverses it and matches across step sizes") {
    Particle oneStep = WithLife(Particle{.velocity = {300.0f, -120.0f}, .drag = 5.0f}, 4.0f);
    Particle halfSteps = oneStep;

    REQUIRE(StepParticle(oneStep, 0.2f));
    REQUIRE(StepParticle(halfSteps, 0.1f));
    REQUIRE(StepParticle(halfSteps, 0.1f));

    CHECK(oneStep.position.x == doctest::Approx(halfSteps.position.x));
    CHECK(oneStep.position.y == doctest::Approx(halfSteps.position.y));
    CHECK(oneStep.velocity.x == doctest::Approx(halfSteps.velocity.x));
    CHECK(oneStep.velocity.y == doctest::Approx(halfSteps.velocity.y));
    CHECK(oneStep.velocity.x > 0.0f);
    CHECK(oneStep.velocity.x < 300.0f);

    // Even a multi-second frame hitch can only decay velocity toward zero, never flip its sign.
    REQUIRE(StepParticle(oneStep, 3.0f));
    CHECK(oneStep.velocity.x >= 0.0f);
    CHECK(oneStep.velocity.y <= 0.0f);
}

TEST_CASE("floating smoke rises, spreads and turns without ever landing on the floor") {
    Particle smoke = WithLife(
        Particle{
            .kind = ParticleKind::Smoke,
            .height = 6.0f,
            .lift = 20.0f,
            .gravity = 0.0f,
            .spin = 4.0f,
            .size = 5.0f,
            .growth = 12.0f,
        },
        1.0f);

    CHECK_FALSE(IsOnFloor(smoke));
    REQUIRE(StepParticle(smoke, 0.25f));
    CHECK(smoke.height == doctest::Approx(11.0f));
    CHECK(smoke.size == doctest::Approx(8.0f));
    CHECK(smoke.angle == doctest::Approx(1.0f));
    CHECK_FALSE(IsOnFloor(smoke));
}

TEST_CASE("a casing bounces off the floor, loses spin and speed, then settles still before expiring") {
    Particle casing = WithLife(
        Particle{
            .kind = ParticleKind::Casing,
            .velocity = {120.0f, 0.0f},
            .drag = 2.0f,
            .height = 14.0f,
            .lift = 90.0f,
            .gravity = 480.0f,
            .bounce = 0.45f,
            .spin = 18.0f,
            .size = 5.0f,
        },
        4.5f);

    CHECK_FALSE(IsOnFloor(casing));

    bool bouncedUp = false;
    bool hitGroundOnce = false;
    for (int i = 0; i < 90 && !IsOnFloor(casing); ++i) {
        const float previousLift = casing.lift;
        REQUIRE(StepParticle(casing, 1.0f / 60.0f));
        if (previousLift < 0.0f && casing.lift > 0.0f) {
            bouncedUp = true;
        }
        if (casing.height == 0.0f) {
            hitGroundOnce = true;
        }
    }

    CHECK(bouncedUp);
    CHECK(hitGroundOnce);
    CHECK(IsOnFloor(casing));
    CHECK(casing.height == 0.0f);
    CHECK(casing.lift == 0.0f);
    CHECK(casing.spin == 0.0f);
    CHECK(casing.secondsLeft > 2.5f);

    // Once settled on the stone floor, floor friction brings any remaining slide to a near stop.
    for (int i = 0; i < 30; ++i) {
        REQUIRE(StepParticle(casing, 1.0f / 60.0f));
    }
    CHECK(horde::Length(casing.velocity) < 1.0f);
}

TEST_CASE("a drop of blood with zero bounce sticks to the floor the first time it lands") {
    Particle drop = WithLife(
        Particle{
            .kind = ParticleKind::Blood,
            .velocity = {80.0f, 0.0f},
            .height = 10.0f,
            .lift = 20.0f,
            .gravity = 560.0f,
            .bounce = 0.0f,
        },
        3.0f);

    for (int i = 0; i < 40 && !IsOnFloor(drop); ++i) {
        REQUIRE(StepParticle(drop, 1.0f / 60.0f));
    }
    CHECK(IsOnFloor(drop));
    CHECK(drop.height == 0.0f);
    CHECK(drop.lift == 0.0f);
}

TEST_CASE("StepParticles with an arena bounces particles off the crypt walls") {
    const Arena arena{{0.0f, 0.0f}, {200.0f, 200.0f}};
    ParticlePool pool;
    pool.Spawn(WithLife(Particle{.position = {198.0f, 2.0f}, .velocity = {300.0f, -300.0f}, .size = 6.0f}, 1.0f));

    StepParticles(pool, arena, 0.1f);
    REQUIRE(pool.Size() == 1);
    CHECK(horde::Contains(arena, pool[0].position));
    CHECK(pool[0].velocity.x < 0.0f);
    CHECK(pool[0].velocity.y > 0.0f);
}

TEST_CASE("a full particle pool discards new particles cleanly without corrupting live ones") {
    ParticlePool pool;
    for (std::size_t i = 0; i < horde::kMaxParticles; ++i) {
        const Particle* spawned =
            pool.Spawn(WithLife(Particle{.position = {static_cast<float>(i), 1.0f}}, 1.0f));
        REQUIRE(spawned != nullptr);
    }
    CHECK(pool.Size() == pool.MaxSize());
    CHECK(pool.Size() == horde::kMaxParticles);

    CHECK(pool.Spawn(WithLife(Particle{.position = {9999.0f, 9999.0f}}, 1.0f)) == nullptr);
    CHECK(pool.Size() == horde::kMaxParticles);
    CHECK(pool[0].position.x == 0.0f);
    CHECK(pool[horde::kMaxParticles - 1].position.x == static_cast<float>(horde::kMaxParticles - 1));

    // Once the live batch expires, the pool accepts particles again.
    StepParticles(pool, 1.1f);
    CHECK(pool.Empty());
    CHECK(pool.Spawn(WithLife(Particle{}, 0.5f)) != nullptr);
}
