#include <array>

#include <doctest/doctest.h>

#include "core/ImpactFeel.h"

using horde::ImpactFeel;
using horde::WorldEvent;
using horde::WorldEventKind;

namespace {

ImpactFeel Feel(WorldEventKind kind, int amount = 0) {
    return horde::FeelOf(WorldEvent{.kind = kind, .amount = amount});
}

constexpr std::array kAllKinds{WorldEventKind::ShotFired,      WorldEventKind::DashStarted,
                               WorldEventKind::PlayerHurt,     WorldEventKind::PlayerKilled,
                               WorldEventKind::BossHurt,       WorldEventKind::ShieldStruck,
                               WorldEventKind::ShieldBroken,   WorldEventKind::HairballKilled,
                               WorldEventKind::BossLanded,     WorldEventKind::BossHitWall,
                               WorldEventKind::FirstPhaseEnded, WorldEventKind::BossRoared,
                               WorldEventKind::BossDefeated};

} // namespace

TEST_CASE("what happens many times a second or starts a dash neither slows nor shakes") {
    for (const WorldEventKind kind : {WorldEventKind::ShotFired, WorldEventKind::DashStarted,
                                      WorldEventKind::BossHurt, WorldEventKind::ShieldStruck}) {
        const ImpactFeel feel = Feel(kind, 10);
        CHECK(feel.slowSeconds == 0.0f);
        CHECK(feel.trauma == 0.0f);
    }
}

TEST_CASE("a heavy hit on the player slows and shakes harder than a light one, and the last one hardest") {
    const ImpactFeel strand = Feel(WorldEventKind::PlayerHurt, 15);
    const ImpactFeel charge = Feel(WorldEventKind::PlayerHurt, horde::kHeavyHitDamage);
    const ImpactFeel death = Feel(WorldEventKind::PlayerKilled, 15);
    CHECK(strand.slowSeconds > 0.0f);
    CHECK(charge.slowSeconds > strand.slowSeconds);
    CHECK(charge.trauma > strand.trauma);
    CHECK(death.slowSeconds > charge.slowSeconds);
    CHECK(death.trauma > charge.trauma);
}

TEST_CASE("the boss's own weight shakes the screen without stopping the fight") {
    CHECK(Feel(WorldEventKind::BossLanded).slowSeconds == 0.0f);
    CHECK(Feel(WorldEventKind::BossLanded).trauma > 0.0f);
    CHECK(Feel(WorldEventKind::BossRoared).slowSeconds == 0.0f);
    CHECK(Feel(WorldEventKind::BossRoared).trauma > 0.0f);
}

TEST_CASE("winning the fight is the biggest moment of all, and every slow-down stays short") {
    const ImpactFeel victory = Feel(WorldEventKind::BossDefeated);
    for (const WorldEventKind kind : kAllKinds) {
        const ImpactFeel feel = Feel(kind, 50);
        CHECK(feel.slowSeconds <= victory.slowSeconds);
        CHECK(feel.trauma <= victory.trauma);
        CHECK(feel.trauma <= 1.0f);
        // Longer than this and a slow-down stops reading as a hit and starts reading as a cutscene.
        CHECK(feel.slowSeconds <= 1.0f);
    }
}
