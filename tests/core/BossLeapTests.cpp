#include <doctest/doctest.h>

#include "core/Boss.h"
#include "core/World.h"

using horde::AttackKind;
using horde::AttackStage;
using horde::Boss;
using horde::Vec2;

namespace {

// The game's arena: (0, 0) to (1640, 1040), centre (820, 520).
constexpr horde::Arena kArena = horde::kBossArena;
constexpr Vec2 kCenter{820.0f, 520.0f};
constexpr float kDt = 1.0f / 60.0f;
constexpr float kPlayerRadius = 20.0f;
constexpr Vec2 kSouth = kCenter + Vec2{0.0f, 300.0f};

// A boss standing at `position` that starts a Leap on the next step.
Boss AboutToLeapFrom(Vec2 position) {
    Boss boss;
    boss.motion = horde::MotionAt(position);
    boss.nextAttack = AttackKind::Leap;
    boss.secondsUntilAttack = 0.0f;
    return boss;
}

// Steps the boss, and returns how many times it landed.
int Step(Boss& boss, Vec2 player, int steps) {
    int landings = 0;
    for (int i = 0; i < steps; ++i) {
        if (horde::StepBoss(boss, player, kPlayerRadius, kArena, kDt).landed) {
            ++landings;
        }
    }
    return landings;
}

bool InStage(const Boss& boss, AttackStage stage) {
    return boss.attack && boss.attack->stage == stage;
}

} // namespace

TEST_CASE("the leap crouches on the ground, then takes off") {
    Boss boss = AboutToLeapFrom(kCenter);
    Step(boss, kSouth, 1);
    REQUIRE(boss.attack);
    REQUIRE(boss.attack->kind == AttackKind::Leap);
    const Vec2 crouched = boss.motion.position;

    // 0.35 s of crouching: still on the ground, still a body, and not moving.
    Step(boss, kSouth, 19);
    CHECK_FALSE(horde::IsAirborne(boss));
    CHECK(horde::Length(boss.motion.position - crouched) < 1.0f);

    Step(boss, kSouth, 3);
    CHECK(horde::IsAirborne(boss));
    CHECK(InStage(boss, AttackStage::Warning));
}

TEST_CASE("the shadow appears under the player, follows them at its speed, then locks") {
    Boss boss = AboutToLeapFrom(kCenter);
    Step(boss, kSouth, 1);
    REQUIRE(boss.attack);
    CHECK(boss.attack->target == kSouth);

    // The player runs east. The shadow waits for the boss to leave the ground, then follows.
    const Vec2 east = kSouth + Vec2{500.0f, 0.0f};
    while (!horde::IsAirborne(boss)) {
        Step(boss, east, 1);
    }
    const float followed = boss.attack->target.x - kSouth.x;
    CHECK(followed == doctest::Approx(boss.leap.shadowSpeed * kDt).epsilon(0.01));

    // 0.65 s of following in all, then it locks and stays put while the player runs back west.
    Step(boss, east, 45);
    const Vec2 locked = boss.attack->target;
    CHECK(locked.x - kSouth.x == doctest::Approx(boss.leap.shadowSpeed * 0.65f).epsilon(0.03));
    Step(boss, kSouth - Vec2{500.0f, 0.0f}, 10);
    REQUIRE(horde::IsAirborne(boss));
    CHECK(boss.attack->target == locked);
}

TEST_CASE("the boss flies to its shadow and lands on it, once") {
    Boss boss = AboutToLeapFrom(kCenter);
    Step(boss, kSouth, 1);
    int landings = 0;
    while (boss.attack && boss.attack->stage == AttackStage::Warning) {
        landings += Step(boss, kSouth, 1);
    }
    CHECK(landings == 1);
    REQUIRE(InStage(boss, AttackStage::Active));
    CHECK(boss.motion.position == boss.attack->target);
    CHECK(horde::Length(boss.motion.position - kSouth) < 1.0f);
    CHECK_FALSE(horde::IsAirborne(boss));
    CHECK(Step(boss, kSouth, 120) == 0);
}

TEST_CASE("the shadow stays where the whole body fits inside the arena") {
    Boss boss = AboutToLeapFrom(kCenter);
    const Vec2 corner{5.0f, 5.0f};
    Step(boss, corner, 1);
    REQUIRE(boss.attack);
    CHECK(boss.attack->target == Vec2{boss.radius, boss.radius});
    Step(boss, corner, 50);
    CHECK(boss.attack->target == Vec2{boss.radius, boss.radius});
}

TEST_CASE("after landing the boss recovers for a second, then stalks with the whole interval ahead") {
    Boss boss = AboutToLeapFrom(kCenter);
    Step(boss, kSouth, 1);
    while (boss.attack && boss.attack->stage == AttackStage::Warning) {
        Step(boss, kSouth, 1);
    }
    // 0.15 s of impact and 1 s of recovery.
    int steps = 0;
    while (boss.attack && steps < 120) {
        Step(boss, kSouth, 1);
        ++steps;
    }
    CHECK(steps == doctest::Approx(69).epsilon(0.05));
    CHECK(boss.secondsUntilAttack == boss.attackInterval);
    CHECK(boss.nextAttack == AttackKind::Charge);
}
