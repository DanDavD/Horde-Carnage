#include <algorithm>
#include <ostream>
#include <string_view>

#include <doctest/doctest.h>

#include "core/Boss.h"
#include "core/World.h"
#include "core/bosses/Matriarch.h"

using horde::Boss;
using horde::BossId;
using horde::Vec2;

namespace {

constexpr float kDt = 1.0f / 60.0f;

} // namespace

TEST_CASE("every boss id has its row in the table of bosses, in the order of the enum") {
    for (int i = 0; i < horde::kBossCount; ++i) {
        const BossId id = static_cast<BossId>(i);
        const horde::BossRules& rules = horde::RulesOf(id);
        CAPTURE(i);
        CHECK(rules.id == id);
        CHECK_FALSE(rules.name.empty());
        CHECK(rules.make != nullptr);
        CHECK(rules.step != nullptr);
    }
}

TEST_CASE("a boss made from the table is the kind asked for, standing still where it was asked to") {
    const Boss boss = horde::MakeBoss(BossId::Matriarch, Vec2{300.0f, 200.0f});
    CHECK(boss.id == BossId::Matriarch);
    CHECK(boss.motion.position == Vec2{300.0f, 200.0f});
    CHECK(boss.motion.velocity == Vec2{});
    CHECK(boss.health.current == boss.health.max);
    CHECK(boss.phase == horde::BossPhase::One);
    CHECK(horde::RulesOf(BossId::Matriarch).name == "The Matriarch");
}

TEST_CASE("a new world starts with the boss it is asked for, in the middle of the arena") {
    const horde::World world = horde::CreateWorld(horde::kBossArena, Vec2{1280.0f, 720.0f}, 7, BossId::Matriarch);
    CHECK(world.boss.id == BossId::Matriarch);
    CHECK(world.boss.motion.position == horde::Center(horde::kBossArena));
}

TEST_CASE("stepping a living boss runs the step in its row of the table") {
    horde::World world = horde::CreateWorld(horde::kBossArena, Vec2{1280.0f, 720.0f}, 7);
    Boss throughTable = world.boss;
    Boss direct = world.boss;
    horde::Random randomA = world.random;
    horde::Random randomB = world.random;
    const horde::Motion& player = world.player.motion;

    for (int i = 0; i < 600; ++i) {
        horde::StepBoss(throughTable, player, world.player.radius, world.arena, randomA, kDt);
        // StepBoss fades the hit flash itself before it hands a living boss to its own step.
        direct.hitFlashSeconds = std::max(direct.hitFlashSeconds - kDt, 0.0f);
        horde::StepMatriarch(direct, player, world.player.radius, world.arena, randomB, kDt);
    }
    CHECK(throughTable.motion.position == direct.motion.position);
    CHECK(throughTable.attack.has_value() == direct.attack.has_value());
    CHECK(randomA.state == randomB.state);
}
