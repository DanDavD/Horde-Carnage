#include "core/Boss.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "core/bosses/Matriarch.h"

namespace horde {

namespace {

// The table of bosses, in the order of `BossId`, which a test checks.
constexpr std::array<BossRules, kBossCount> kBosses{{
    {.id = BossId::Matriarch, .name = "The Matriarch", .make = MakeMatriarch, .step = StepMatriarch},
}};

} // namespace

BossActions StepBoss(Boss& boss, const Motion& player, float playerRadius, const Arena& arena, Random& random,
                     float dt) {
    boss.hitFlashSeconds = std::max(boss.hitFlashSeconds - dt, 0.0f);

    // A dead boss drops whatever it was doing and only brakes to a stop.
    if (IsDead(boss.health)) {
        boss.attack.reset();
        StepMovement(boss.motion, Vec2{}, boss.stats, dt);
        return {};
    }
    return RulesOf(boss.id).step(boss, player, playerRadius, arena, random, dt);
}

const BossRules& RulesOf(BossId id) {
    return kBosses[static_cast<std::size_t>(id)];
}

Boss MakeBoss(BossId id, Vec2 position) {
    return RulesOf(id).make(position);
}

bool IsCharging(const Boss& boss) {
    return boss.attack && boss.attack->kind == AttackKind::Charge && boss.attack->clock.stage == AttackStage::Active;
}

bool IsAirborne(const Boss& boss) {
    const std::optional<BossAttack>& attack = boss.attack;
    return attack && attack->kind == AttackKind::Leap && IsLeapAirborne(boss.leap, *attack);
}

float WarningSeconds(const Boss& boss, const BossAttack& attack) {
    switch (attack.kind) {
    case AttackKind::Charge:
        return ChargeWarningSeconds(boss.charge, attack);
    case AttackKind::HairRing:
        return HairRingWarningSeconds(boss.hairRing, attack);
    case AttackKind::Leap:
        return LeapCrouchSeconds(boss, attack) + LeapAirSeconds(boss, attack);
    case AttackKind::Volley:
        return VolleyWarningSeconds(boss.volley, attack);
    case AttackKind::Summon:
        return SummonWarningSeconds(boss.summon, attack);
    }
    return 0.0f;
}

float LeapCrouchSeconds(const Boss& boss, const BossAttack& leap) {
    return LeapCrouchSeconds(boss.leap, leap);
}

float LeapAirSeconds(const Boss& boss, const BossAttack& leap) {
    return LeapAirSeconds(boss.leap, leap);
}

Projectile MakeHairStrand(const Boss& boss, Vec2 direction) {
    return MakeHairStrand(boss.hairRing, boss.motion.position, boss.radius, direction);
}

Vec2 LeadPoint(const Boss& boss, const Motion& player, const Arena& arena) {
    return LeadPoint(boss.volley, boss.motion.position, boss.radius, player, arena);
}

Projectile MakeVolleyStrand(const Boss& boss, Vec2 direction) {
    return MakeVolleyStrand(boss.volley, boss.motion.position, boss.radius, direction);
}

} // namespace horde
