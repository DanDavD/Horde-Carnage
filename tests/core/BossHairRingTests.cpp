#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include <doctest/doctest.h>

#include "core/Boss.h"
#include "core/World.h"

using horde::AttackKind;
using horde::AttackStage;
using horde::Boss;
using horde::HairRingStats;
using horde::Projectile;
using horde::Vec2;

namespace {

// The game's arena: (0, 0) to (1640, 1040), centre (820, 520).
constexpr horde::Arena kArena = horde::kBossArena;
constexpr Vec2 kCenter{820.0f, 520.0f};
constexpr float kDt = 1.0f / 60.0f;
constexpr float kPlayerRadius = 20.0f;
// The player south of the boss, where the fight starts.
constexpr Vec2 kSouth = kCenter + Vec2{0.0f, 300.0f};

// A boss standing at `position` that starts a Hair Ring on the next step.
Boss AboutToSpitFrom(Vec2 position) {
    Boss boss;
    boss.motion = horde::MotionAt(position);
    boss.nextAttack = AttackKind::HairRing;
    boss.secondsUntilAttack = 0.0f;
    return boss;
}

// Steps the boss, and returns how many rings it spat.
int Step(Boss& boss, Vec2 player, int steps) {
    int rings = 0;
    for (int i = 0; i < steps; ++i) {
        if (horde::StepBoss(boss, player, kPlayerRadius, kArena, kDt).spitHairRing) {
            ++rings;
        }
    }
    return rings;
}

// Steps until the boss spits a ring, for at most five seconds, and returns how many steps that took.
int StepsUntilSpit(Boss& boss, Vec2 player) {
    for (int steps = 1; steps <= 300; ++steps) {
        if (Step(boss, player, 1) == 1) {
            return steps;
        }
    }
    FAIL("the boss never spat");
    return 0;
}

bool InStage(const Boss& boss, AttackStage stage) {
    return boss.attack && boss.attack->stage == stage;
}

// The angle between two directions of length 1, from 0 to 180 degrees.
float DegreesBetween(Vec2 a, Vec2 b) {
    return std::acos(std::clamp(horde::Dot(a, b), -1.0f, 1.0f)) * 180.0f / std::numbers::pi_v<float>;
}

} // namespace

TEST_CASE("the attacks take turns: a charge, a hair ring, a leap, then a charge again") {
    Boss boss;
    boss.motion = horde::MotionAt(kCenter);
    std::vector<AttackKind> started;
    bool attacking = false;
    for (int i = 0; i < 60 * 40 && started.size() < 4; ++i) {
        Step(boss, kSouth, 1);
        if (boss.attack && !attacking) {
            started.push_back(boss.attack->kind);
        }
        attacking = boss.attack.has_value();
    }
    REQUIRE(started.size() == 4);
    CHECK(started[0] == AttackKind::Charge);
    CHECK(started[1] == AttackKind::HairRing);
    CHECK(started[2] == AttackKind::Leap);
    CHECK(started[3] == AttackKind::Charge);
}

TEST_CASE("during the warning the boss holds its ground, and the gap opens 90 degrees from the player") {
    Boss boss = AboutToSpitFrom(kCenter);
    Step(boss, kSouth, 1);
    REQUIRE(InStage(boss, AttackStage::Warning));
    REQUIRE(boss.attack->kind == AttackKind::HairRing);
    const Vec2 gap = boss.attack->direction;
    CHECK(horde::Length(gap) == doctest::Approx(1.0f));
    CHECK(DegreesBetween(gap, Vec2{0.0f, 1.0f}) == doctest::Approx(90.0f));

    // The player runs east. The gap stays where it was shown, and the boss stays put.
    const Vec2 where = boss.motion.position;
    Step(boss, kCenter + Vec2{300.0f, 0.0f}, 30);
    REQUIRE(InStage(boss, AttackStage::Warning));
    CHECK(boss.attack->direction == gap);
    CHECK(horde::Length(boss.motion.position - where) < 1.0f);
}

TEST_CASE("the gap opens on one side of the player, and on the other side the next time") {
    Boss boss = AboutToSpitFrom(kCenter);
    Step(boss, kSouth, 1);
    REQUIRE(boss.attack);
    const Vec2 first = boss.attack->direction;
    for (int i = 0; i < 300 && boss.attack; ++i) {
        Step(boss, kSouth, 1);
    }
    REQUIRE_FALSE(boss.attack);

    boss.nextAttack = AttackKind::HairRing;
    boss.secondsUntilAttack = 0.0f;
    Step(boss, kSouth, 1);
    REQUIRE(boss.attack);
    CHECK(DegreesBetween(first, boss.attack->direction) == doctest::Approx(180.0f).epsilon(0.01));
    CHECK(boss.ringsStarted == 2);
}

TEST_CASE("the ring is spat once, as the warning ends, and the boss stalks again after the recovery") {
    Boss boss = AboutToSpitFrom(kCenter);
    Step(boss, kSouth, 1);
    // 0.6 s of warning, then the first step of the active stage spits.
    CHECK(StepsUntilSpit(boss, kSouth) == doctest::Approx(37).epsilon(0.05));
    CHECK(InStage(boss, AttackStage::Active));

    // 0.15 s of spitting and 0.8 s of recovery, with no second ring.
    int rings = 0;
    int steps = 0;
    while (boss.attack && steps < 120) {
        rings += Step(boss, kSouth, 1);
        ++steps;
    }
    CHECK(rings == 0);
    CHECK(steps == doctest::Approx(57).epsilon(0.05));
    CHECK(boss.secondsUntilAttack == boss.attackInterval);
    CHECK(boss.nextAttack == AttackKind::Leap);
}

TEST_CASE("a ring has a strand in every slot but the gap, evenly spaced, and none inside the gap") {
    const HairRingStats stats;
    const int strands = horde::HairRingStrands(stats);
    REQUIRE(strands == 42);
    const float slot = 360.0f / static_cast<float>(stats.slots);
    // The two strands on the edges of a 45 degree gap of six slots, 3.5 slots from its middle.
    const float edge = 3.5f * slot;

    const Vec2 gap{0.0f, 1.0f};
    std::vector<Vec2> directions;
    for (int i = 0; i < strands; ++i) {
        directions.push_back(horde::HairStrandDirection(gap, i, stats));
    }
    for (std::size_t i = 0; i < directions.size(); ++i) {
        CHECK(horde::Length(directions[i]) == doctest::Approx(1.0f));
        CHECK(DegreesBetween(directions[i], gap) >= edge - 0.01f);
        if (i + 1 < directions.size()) {
            CHECK(DegreesBetween(directions[i], directions[i + 1]) == doctest::Approx(slot).epsilon(0.001));
        }
    }
    CHECK(DegreesBetween(directions.front(), gap) == doctest::Approx(edge).epsilon(0.001));
    CHECK(DegreesBetween(directions.back(), gap) == doctest::Approx(edge).epsilon(0.001));
}

TEST_CASE("a strand leaves the edge of the body and flies at the ring's speed for its range") {
    Boss boss;
    boss.motion = horde::MotionAt(kCenter);
    const HairRingStats& stats = boss.hairRing;

    const Projectile strand = horde::MakeHairStrand(boss, Vec2{1.0f, 0.0f});
    CHECK(strand.motion.position == kCenter + Vec2{boss.radius, 0.0f});
    CHECK(strand.motion.velocity == Vec2{stats.speed, 0.0f});
    CHECK(strand.radius == stats.strandRadius);
    CHECK(strand.damage == stats.damage);
    CHECK(strand.secondsLeft == doctest::Approx(stats.range / stats.speed));
}

TEST_CASE("with more waves, each ring leaves after the wave interval and turns the gap") {
    Boss boss = AboutToSpitFrom(kCenter);
    boss.hairRing.waves = 3;
    boss.hairRing.gapTurnDegrees = 30.0f;
    Step(boss, kSouth, 1);

    StepsUntilSpit(boss, kSouth);
    const Vec2 first = boss.attack->direction;
    CHECK(StepsUntilSpit(boss, kSouth) == doctest::Approx(24).epsilon(0.05)); // 0.4 s later
    const Vec2 second = boss.attack->direction;
    CHECK(DegreesBetween(first, second) == doctest::Approx(30.0f).epsilon(0.001));
    StepsUntilSpit(boss, kSouth);
    CHECK(DegreesBetween(second, boss.attack->direction) == doctest::Approx(30.0f).epsilon(0.001));

    // Three waves and no more.
    int rings = 0;
    for (int i = 0; i < 120 && boss.attack; ++i) {
        rings += Step(boss, kSouth, 1);
    }
    CHECK(rings == 0);
    CHECK_FALSE(boss.attack);
}
