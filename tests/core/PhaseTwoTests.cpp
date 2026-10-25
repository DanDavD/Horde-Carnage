#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/AttackChoice.h"
#include "core/World.h"

using horde::AttackKind;
using horde::Boss;
using horde::BossPhase;
using horde::PlayerCommands;
using horde::Vec2;
using horde::World;

namespace {

constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

World NewFight() {
    World world = horde::CreateWorld(horde::kBossArena, kView);
    world.boss = horde::test::Predictable(world.boss);
    return world;
}

void Step(World& world, const PlayerCommands& commands, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepWorld(world, commands, kDt);
    }
}

// Fires one pistol bullet at the boss, which the player starts out facing, and waits for it to land.
void ShootBossOnce(World& world) {
    const int hitsBefore = world.shotsHit;
    Step(world, PlayerCommands{.firePressed = true}, 1);
    for (int i = 0; i < 40 && world.shotsHit == hitsBefore; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    REQUIRE(world.shotsHit == hitsBefore + 1);
}

} // namespace

TEST_CASE("emptying the first phase's bar starts the transformation, not the victory") {
    World world = NewFight();
    Boss& boss = world.boss;
    horde::BreakShield(boss.shield);
    boss.health.current = horde::kPistol.damage;
    ShootBossOnce(world);

    CHECK(boss.phase == BossPhase::Transforming);
    CHECK(horde::IsTransforming(boss));
    CHECK(boss.health.current == boss.health.max);
    CHECK_FALSE(horde::IsDead(boss.health));
    CHECK_FALSE(horde::IsBossDefeated(world));
    CHECK_FALSE(boss.attack);
}

TEST_CASE("while it transforms the boss cannot be hurt, holds still and does not attack") {
    World world = NewFight();
    Boss& boss = world.boss;
    horde::TransformBoss(world);
    const Vec2 where = boss.motion.position;
    const float shield = boss.shield.strength;

    // Pistol shots for most of the two seconds: they reach the boss and do nothing.
    for (int i = 0; i < 110; ++i) {
        Step(world, PlayerCommands{.firePressed = i % 10 == 0}, 1);
    }
    REQUIRE(horde::IsTransforming(boss));
    CHECK(world.shotsHit > 3);
    CHECK(boss.health.current == boss.health.max);
    CHECK(boss.shield.strength == shield);
    CHECK_FALSE(boss.attack);
    CHECK(world.hostileProjectiles.Size() == 0);
    CHECK(horde::Length(boss.motion.position - where) < 1.0f);
}

TEST_CASE("the transformation ends with a roar, a fresh shield and the second phase's numbers") {
    World world = NewFight();
    Boss& boss = world.boss;
    horde::BreakShield(boss.shield);
    horde::TransformBoss(world);

    int steps = 0;
    while (horde::IsTransforming(boss) && steps < 200) {
        Step(world, PlayerCommands{}, 1);
        ++steps;
    }
    CHECK(steps == doctest::Approx(120).epsilon(0.02));
    REQUIRE(boss.phase == BossPhase::Two);
    // The roar: one shockwave from the body.
    CHECK(world.shockwaves.Size() == 1);
    // A fresh Shell, with the shorter opening.
    CHECK(horde::IsShieldUp(boss.shield));
    CHECK(boss.shield.kind == horde::ShieldKind::Shell);
    CHECK(boss.shield.strength == doctest::Approx(boss.shield.stats.shellStrength));
    CHECK(boss.shield.stats.exposedSeconds == doctest::Approx(boss.phaseTwo.exposedSeconds));
    // Shorter stalks, more combos, and the Volley next.
    CHECK(boss.choice.maxStalkSeconds == doctest::Approx(boss.phaseTwo.choice.maxStalkSeconds));
    CHECK(boss.combo.chance == doctest::Approx(boss.phaseTwo.combo.chance));
    CHECK(boss.nextAttack == AttackKind::Volley);

    Step(world, PlayerCommands{}, 60);
    REQUIRE(boss.attack);
    CHECK(boss.attack->kind == AttackKind::Volley);
}

TEST_CASE("the roar's shockwave catches a player who waits next to the boss") {
    World world = NewFight();
    horde::TransformBoss(world);
    const float contact = world.boss.radius + world.player.radius;
    world.player.motion = horde::MotionAt(world.boss.motion.position + Vec2{0.0f, contact + 30.0f});
    for (int i = 0; i < 160; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    CHECK(world.player.health.current < world.player.health.max);
    CHECK(world.lastHitBy == horde::DamageSource::Shockwave);
}

TEST_CASE("the roar dies out before the distance the fight starts at") {
    // The player stays where the fight puts them, 400 units south, through the roar and a little after.
    World world = NewFight();
    world.boss.stats.maxSpeed = 0.0f;
    horde::TransformBoss(world);
    for (int i = 0; i < 160; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    REQUIRE(world.boss.phase == BossPhase::Two);
    CHECK(world.player.health.current == world.player.health.max);
}

TEST_CASE("only an empty bar in the second phase wins the fight") {
    World world = NewFight();
    Boss& boss = world.boss;
    boss.phase = BossPhase::Two;
    horde::BreakShield(boss.shield);
    boss.health.current = horde::kPistol.damage;
    ShootBossOnce(world);
    CHECK(horde::IsDead(boss.health));
    CHECK(horde::IsBossDefeated(world));
}

TEST_CASE("the Debug shortcuts skip to the transformation and to the victory") {
    World transformed = NewFight();
    horde::TransformBoss(transformed);
    CHECK(horde::IsTransforming(transformed.boss));
    // Only from the first phase: a second press does not start it again.
    Step(transformed, PlayerCommands{}, 60);
    const float left = transformed.boss.transformSecondsLeft;
    horde::TransformBoss(transformed);
    CHECK(transformed.boss.transformSecondsLeft == left);

    World won = NewFight();
    horde::DefeatBoss(won);
    CHECK(horde::IsBossDefeated(won));
}

TEST_CASE("the Volley joins the picks in the second phase, and never shows in the first") {
    horde::Random random = horde::SeedRandom(61);
    const horde::AttackChoiceStats phaseOne;
    const horde::AttackChoiceStats phaseTwo = horde::PhaseTwoChoice();
    int phaseOneVolleys = 0;
    int phaseTwoVolleys = 0;
    for (int i = 0; i < 1000; ++i) {
        phaseOneVolleys += horde::ChooseAttack(phaseOne, 900.0f, std::nullopt, random) == AttackKind::Volley ? 1 : 0;
        phaseTwoVolleys += horde::ChooseAttack(phaseTwo, 900.0f, std::nullopt, random) == AttackKind::Volley ? 1 : 0;
    }
    CHECK(phaseOneVolleys == 0);
    // Far away the weights are 4, 1, 2 and 3: three picks in ten are a Volley.
    CHECK(phaseTwoVolleys > 250);
    CHECK(phaseTwoVolleys < 350);
}
