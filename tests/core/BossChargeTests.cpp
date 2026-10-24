#include <cmath>

#include <doctest/doctest.h>

#include "core/Boss.h"
#include "core/World.h"

using horde::AttackStage;
using horde::Boss;
using horde::ChargePath;
using horde::Vec2;

namespace {

// The game's arena: (0, 0) to (1640, 1040), centre (820, 520).
constexpr horde::Arena kArena = horde::kBossArena;
constexpr Vec2 kCenter{820.0f, 520.0f};
constexpr float kDt = 1.0f / 60.0f;
constexpr float kPlayerRadius = 20.0f;

// A boss standing at `position`, at the very start of its charge warning.
Boss AboutToChargeFrom(Vec2 position) {
    Boss boss;
    boss.motion = horde::MotionAt(position);
    boss.attack = horde::BossAttack{};
    return boss;
}

void Step(Boss& boss, Vec2 player, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepBoss(boss, player, kPlayerRadius, kArena, kDt);
    }
}

bool InStage(const Boss& boss, AttackStage stage) {
    return boss.attack && boss.attack->stage == stage;
}

// Steps until the attack reaches `stage`, for at most five seconds.
void StepUntil(Boss& boss, Vec2 player, AttackStage stage) {
    for (int i = 0; i < 300 && !InStage(boss, stage); ++i) {
        Step(boss, player, 1);
    }
    REQUIRE(InStage(boss, stage));
}

} // namespace

TEST_CASE("the boss stalks the player, then starts a charge after its attack interval") {
    Boss boss;
    boss.motion = horde::MotionAt(kCenter);
    const Vec2 player = kCenter + Vec2{0.0f, 400.0f};

    Step(boss, player, 118); // Just short of the two seconds.
    CHECK_FALSE(boss.attack);
    CHECK(boss.motion.position.y > kCenter.y);

    Step(boss, player, 4);
    REQUIRE(boss.attack);
    CHECK(boss.attack->kind == horde::AttackKind::Charge);
    CHECK(boss.attack->stage == AttackStage::Warning);
}

TEST_CASE("during the warning the boss stands still, and its aim follows the player until it locks") {
    Boss boss = AboutToChargeFrom(kCenter);
    const Vec2 east = kCenter + Vec2{300.0f, 0.0f};
    Step(boss, east, 1);
    REQUIRE(InStage(boss, AttackStage::Warning));

    // The player moves north: the path swings to follow.
    const Vec2 north = kCenter + Vec2{0.0f, -300.0f};
    Step(boss, north, 20);
    CHECK(boss.attack->direction.y == doctest::Approx(-1.0f));

    // Past the lock (0.5 s into the 0.8 s warning) the player runs east again, and the path stays put.
    Step(boss, north, 15);
    Step(boss, east, 10);
    REQUIRE(InStage(boss, AttackStage::Warning));
    CHECK(boss.attack->direction.y == doctest::Approx(-1.0f));
    CHECK(boss.motion.position == kCenter);
}

TEST_CASE("the charge runs in a straight line at its speed") {
    Boss boss = AboutToChargeFrom(kCenter);
    const Vec2 player = kCenter + Vec2{300.0f, 0.0f};
    StepUntil(boss, player, AttackStage::Active);
    CHECK(horde::IsCharging(boss));
    const Vec2 before = boss.motion.position;

    Step(boss, player, 1);
    CHECK(boss.motion.position.x == doctest::Approx(before.x + boss.charge.speed * kDt));
    CHECK(boss.motion.position.y == doctest::Approx(before.y));
    CHECK(boss.motion.velocity.x == doctest::Approx(boss.charge.speed));
}

TEST_CASE("a charge into a wall stops at the wall and stuns the boss for the long recovery") {
    // From the middle, the east wall is 760 units away for the body, inside the 800 unit run.
    Boss boss = AboutToChargeFrom(kCenter);
    const Vec2 player = kCenter + Vec2{300.0f, 0.0f};
    StepUntil(boss, player, AttackStage::Recovery);
    CHECK(boss.attack->stunned);
    CHECK(boss.motion.position.x == doctest::Approx(kArena.max.x - boss.radius));
    CHECK_FALSE(horde::IsCharging(boss));

    Step(boss, player, 60); // Past the short recovery, inside the stun.
    CHECK(InStage(boss, AttackStage::Recovery));
    // The other half second of the stun, then it is back to stalking, with the whole interval ahead.
    int steps = 0;
    while (boss.attack && steps < 60) {
        Step(boss, player, 1);
        ++steps;
    }
    CHECK(steps == doctest::Approx(30).epsilon(0.05));
    CHECK(boss.secondsUntilAttack == boss.attackInterval);
}

TEST_CASE("a charge that runs out in the open stops after its distance and recovers quickly") {
    const Vec2 start{200.0f, kCenter.y};
    Boss boss = AboutToChargeFrom(start);
    const Vec2 player = start + Vec2{300.0f, 0.0f};
    StepUntil(boss, player, AttackStage::Recovery);
    CHECK_FALSE(boss.attack->stunned);
    CHECK(boss.motion.position.x == doctest::Approx(start.x + boss.charge.distance));

    Step(boss, player, 31);
    CHECK_FALSE(boss.attack);
}

TEST_CASE("a charge path runs its full distance in the open and stops at the first wall") {
    const float radius = Boss{}.radius;

    const ChargePath open = horde::PlanChargePath(kCenter, Vec2{1.0f, 0.0f}, 300.0f, radius, kArena);
    CHECK_FALSE(open.endsAtWall);
    CHECK(open.end == kCenter + Vec2{300.0f, 0.0f});

    const ChargePath east = horde::PlanChargePath(kCenter, Vec2{1.0f, 0.0f}, 2000.0f, radius, kArena);
    CHECK(east.endsAtWall);
    CHECK(east.end.x == doctest::Approx(kArena.max.x - radius));

    // Diagonally down and right, the south wall comes before the east one.
    const Vec2 diagonal = horde::Normalized(Vec2{1.0f, 1.0f});
    const ChargePath corner = horde::PlanChargePath(kCenter, diagonal, 2000.0f, radius, kArena);
    CHECK(corner.endsAtWall);
    CHECK(corner.end.y == doctest::Approx(kArena.max.y - radius));
    CHECK(corner.end.x == doctest::Approx(kCenter.x + (kArena.max.y - radius - kCenter.y)));
}

TEST_CASE("a dead boss drops its attack and stays where it fell") {
    Boss boss = AboutToChargeFrom(kCenter);
    const Vec2 player = kCenter + Vec2{300.0f, 0.0f};
    StepUntil(boss, player, AttackStage::Warning);
    boss.health.current = 0;

    Step(boss, player, 120);
    CHECK_FALSE(boss.attack);
    CHECK(boss.motion.position == kCenter);
}
