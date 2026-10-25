#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/Boss.h"
#include "core/World.h"

using horde::AttackKind;
using horde::AttackStage;
using horde::Boss;
using horde::BossAttack;
using horde::BossPhase;
using horde::PlayerCommands;
using horde::Projectile;
using horde::Vec2;
using horde::World;

namespace {

constexpr horde::Arena kArena = horde::kBossArena;
constexpr Vec2 kCenter{820.0f, 520.0f};
constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;
constexpr float kPlayerRadius = 20.0f;

// What the bosses below draw from. With their chance taken out, it only decides the stalks between attacks.
horde::Random chance = horde::SeedRandom(5);

// A fight in the second phase, with the boss's shield down and its chance taken out.
World SecondPhase() {
    World world = horde::CreateWorld(kArena, kView);
    world.boss = horde::test::Predictable(world.boss);
    world.boss.phase = BossPhase::Two;
    horde::BreakShield(world.boss.shield);
    return world;
}

// Fires one pistol bullet at the boss, which the player starts out facing, and waits for it to land.
void ShootBossOnce(World& world) {
    const int hitsBefore = world.shotsHit;
    horde::StepWorld(world, PlayerCommands{.firePressed = true}, kDt);
    for (int i = 0; i < 40 && world.shotsHit == hitsBefore; ++i) {
        horde::StepWorld(world, PlayerCommands{}, kDt);
    }
    REQUIRE(world.shotsHit == hitsBefore + 1);
}

// A frenzied boss without its chance in the middle, at the very start of `attack`.
Boss FrenziedStarting(BossAttack attack) {
    Boss boss = horde::test::Predictable(Boss{});
    boss.phase = BossPhase::Two;
    horde::EnterFrenzy(boss);
    boss.combo.chance = 0.0f;
    boss.motion = horde::MotionAt(kCenter);
    boss.attack = attack;
    return boss;
}

int CountFrenzyStrands(const World& world) {
    int count = 0;
    for (const Projectile& strand : world.hostileProjectiles) {
        count += strand.source == horde::DamageSource::Frenzy ? 1 : 0;
    }
    return count;
}

} // namespace

TEST_CASE("the bullet that takes the second phase's bar to half throws the boss into a frenzy, with a roar") {
    World world = SecondPhase();
    Boss& boss = world.boss;
    boss.health.current = boss.health.max / 2 + horde::kPistol.damage + 1;
    ShootBossOnce(world);
    CHECK_FALSE(boss.frenzied); // Still a bullet above half.

    ShootBossOnce(world);
    REQUIRE(boss.frenzied);
    CHECK(world.shockwaves.Size() == 1);
    // Every attack gains something, and the rhythm quickens.
    CHECK(boss.charge.boltIntervalSeconds == doctest::Approx(boss.frenzy.chargeBoltIntervalSeconds));
    CHECK(boss.hairRing.waves == boss.frenzy.ringWaves);
    CHECK(boss.leap.landingStrands == boss.frenzy.leapLandingStrands);
    CHECK(boss.leap.shadowSpeed == doctest::Approx(boss.frenzy.leapShadowSpeed));
    CHECK(boss.volley.bursts == boss.frenzy.volleyBursts);
    CHECK(boss.choice.maxStalkSeconds == doctest::Approx(boss.frenzy.choice.maxStalkSeconds));
    CHECK(boss.combo.chance == doctest::Approx(boss.frenzy.combo.chance));

    // Only once: the next bullet does not roar again.
    ShootBossOnce(world);
    CHECK(world.shockwaves.Size() == 1);
}

TEST_CASE("the first phase never falls into a frenzy") {
    World world = SecondPhase();
    world.boss.phase = BossPhase::One;
    world.boss.health.current = world.boss.health.max / 2;
    ShootBossOnce(world);
    CHECK_FALSE(world.boss.frenzied);
    CHECK_FALSE(horde::ShouldFrenzy(world.boss));
}

TEST_CASE("a frenzied charge throws a pair of strands on a beat while it runs, one at the player, one ahead") {
    World world = SecondPhase();
    world.boss = FrenziedStarting(BossAttack{.kind = AttackKind::Charge});
    // The player south of the charge's path, running west; the charge runs east into the wall.
    world.player.motion = horde::MotionAt(kCenter + Vec2{300.0f, 250.0f});
    world.boss.attack->direction = Vec2{1.0f, 0.0f};
    world.boss.charge.aimLockSeconds = world.boss.charge.warningSeconds;
    int pairs = 0;
    for (int i = 0; i < 120 && world.boss.attack && world.boss.attack->stage != AttackStage::Recovery; ++i) {
        const int before = CountFrenzyStrands(world);
        horde::StepWorld(world, PlayerCommands{.move = {-1.0f, 0.0f}}, kDt);
        if (CountFrenzyStrands(world) == before + 2) {
            ++pairs;
        }
    }
    // A run of 760 units at 1000 units/s is 0.76 s: a pair at 0, 0.15, ... 0.75 s.
    CHECK(pairs == 6);
}

TEST_CASE("a frenzied hair ring spits two rings, the gap turning between them") {
    Boss boss = FrenziedStarting(BossAttack{.kind = AttackKind::HairRing});
    int rings = 0;
    for (int i = 0; i < 300 && boss.attack; ++i) {
        rings += horde::StepBoss(boss, horde::MotionAt(kCenter + Vec2{0.0f, 300.0f}), kPlayerRadius, kArena, chance,
                                 kDt)
                         .spitHairRing
                     ? 1
                     : 0;
    }
    CHECK(rings == 2);
}

TEST_CASE("a frenzied leap lands with a ring of strands with no gap, one of them at the player") {
    World world = SecondPhase();
    world.boss = FrenziedStarting(BossAttack{.kind = AttackKind::Leap});
    world.boss.attack->takeoff = kCenter;
    world.boss.attack->target = kCenter;
    world.boss.leap.shadowSpeed = 0.0f;
    world.player.motion = horde::MotionAt(kCenter + Vec2{0.0f, 300.0f});
    for (int i = 0; i < 120 && CountFrenzyStrands(world) == 0; ++i) {
        horde::StepWorld(world, PlayerCommands{}, kDt);
    }
    REQUIRE(CountFrenzyStrands(world) == world.boss.frenzy.leapLandingStrands);
    bool atPlayer = false;
    for (const Projectile& strand : world.hostileProjectiles) {
        const Vec2 direction = horde::Normalized(strand.motion.velocity);
        atPlayer = atPlayer || (direction.y > 0.999f);
    }
    CHECK(atPlayer);
}

TEST_CASE("a frenzied volley fires five bursts, led every other one") {
    Boss boss = FrenziedStarting(BossAttack{.kind = AttackKind::Volley});
    horde::Motion player = horde::MotionAt(kCenter + Vec2{-300.0f, 300.0f});
    player.velocity = Vec2{300.0f, 0.0f};
    int bursts = 0;
    int led = 0;
    for (int i = 0; i < 300 && boss.attack; ++i) {
        player.position += player.velocity * kDt;
        if (horde::StepBoss(boss, player, kPlayerRadius, kArena, chance, kDt).spitVolley) {
            ++bursts;
            const Vec2 atPlayer = horde::Normalized(player.position - kCenter);
            led += boss.attack->direction.x > atPlayer.x + 0.05f ? 1 : 0;
        }
    }
    CHECK(bursts == 5);
    CHECK(led == 2);
}

TEST_CASE("the Debug shortcut throws the second phase into its frenzy, and does nothing in the first") {
    World first = SecondPhase();
    first.boss.phase = BossPhase::One;
    horde::FrenzyBoss(first);
    CHECK_FALSE(first.boss.frenzied);

    World second = SecondPhase();
    horde::FrenzyBoss(second);
    CHECK(second.boss.frenzied);
    CHECK(second.boss.health.current <= second.boss.health.max / 2);
    CHECK(second.shockwaves.Size() == 1);
}
