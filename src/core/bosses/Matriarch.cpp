#include "core/bosses/Matriarch.h"

#include <algorithm>

#include "core/Chase.h"
#include "core/Collision.h"

namespace horde {

namespace {

// The boss's body, as the attacks in core/attacks/ see it.
Attacker AttackerOf(Boss& boss) {
    return Attacker{.motion = boss.motion,
                    .walk = boss.stats,
                    .radius = boss.radius,
                    .comboRecoverySeconds = boss.combo.finalRecoverySeconds};
}

// Every attack starts with its random picks in the same order (the warning's length, then the combo), so
// the same seed always plays out the same way.
void StartAttack(Boss& boss, AttackKind kind, bool endsCombo, Vec2 playerPosition, const Arena& arena,
                 Random& random) {
    const Vec2 toPlayer = AimAt(boss.motion.position, playerPosition, Vec2{0.0f, 1.0f});
    BossAttack attack{.kind = kind, .direction = toPlayer};
    attack.warningScale = RandomRange(random, boss.choice.minWarningScale, boss.choice.maxWarningScale);
    if (endsCombo) {
        attack.warningScale *= boss.combo.warningScale;
        attack.endsCombo = true;
    } else {
        attack.followUp = ChooseFollowUp(boss.combo, kind, random);
    }
    switch (kind) {
    case AttackKind::Charge:
        break;
    case AttackKind::HairRing:
        // To one side of the player, and to the other side the next time, so reaching the gap always means
        // moving, and the player cannot settle into one habit.
        AimHairRing(attack, toPlayer, boss.ringsStarted % 2 != 0, boss.hairRing);
        ++boss.ringsStarted;
        break;
    case AttackKind::Leap:
        // The shadow appears under the player, and the jump starts from where the boss stands.
        AimLeap(attack, boss.motion.position, playerPosition, boss.radius, arena);
        break;
    case AttackKind::Volley:
    case AttackKind::Summon:
        break;
    }
    boss.lastAttack = kind;
    boss.attack = attack;
}

// Back to stalking, for a stretch the player cannot count down to.
void FinishAttack(Boss& boss, Random& random) {
    boss.attack.reset();
    boss.secondsUntilAttack = RandomRange(random, boss.choice.minStalkSeconds, boss.choice.maxStalkSeconds);
}

// Walks at the player, like a big, slow hairball, until it is time to attack. The attack is picked only
// then, from where the player stands at that moment.
//
// @return Whether an attack started.
bool Stalk(Boss& boss, Vec2 playerPosition, float playerRadius, const Arena& arena, Random& random, float dt) {
    const float stopDistance = boss.radius + playerRadius + kContactSlack;
    StepMovement(boss.motion, ChaseInput(boss.motion.position, playerPosition, stopDistance), boss.stats, dt);

    boss.secondsUntilAttack -= dt;
    if (boss.secondsUntilAttack <= 0.0f) {
        const float distance = Length(playerPosition - boss.motion.position);
        const AttackKind kind =
            boss.nextAttack ? *boss.nextAttack : ChooseAttack(boss.choice, distance, boss.lastAttack, random);
        boss.nextAttack.reset();
        StartAttack(boss, kind, false, playerPosition, arena, random);
        return true;
    }
    return false;
}

// The roar ends the transformation: the second phase's numbers replace the first's, a fresh Shell comes up
// with the shorter opening, and the first attack of the new phase is the new one, so the player sees what
// changed at once.
BossActions EnterPhaseTwo(Boss& boss) {
    const PhaseTwoStats& two = boss.phaseTwo;
    boss.phase = BossPhase::Two;
    boss.transformSecondsLeft = 0.0f;
    boss.choice = two.choice;
    boss.combo = two.combo;
    boss.shield = Shield{};
    boss.shield.stats.exposedSeconds = two.exposedSeconds;
    boss.nextAttack = AttackKind::Volley;
    boss.secondsUntilAttack = two.choice.minStalkSeconds;
    return BossActions{.roared = true};
}

BossActions StepAttack(Boss& boss, const Motion& player, const Arena& arena, Random& random, float dt) {
    const Vec2 playerPosition = player.position;
    BossAttack& attack = *boss.attack;
    switch (attack.kind) {
    case AttackKind::Charge: {
        const ChargeStep step = StepCharge(attack, AttackerOf(boss), boss.charge, playerPosition, arena, dt);
        if (step.finished) {
            FinishAttack(boss, random);
        }
        return BossActions{.throwBolts = step.throwBolts, .hitWall = step.hitWall};
    }
    case AttackKind::HairRing: {
        const HairRingStep step = StepHairRing(attack, AttackerOf(boss), boss.hairRing, dt);
        if (step.finished) {
            FinishAttack(boss, random);
        }
        return BossActions{.spitHairRing = step.spit};
    }
    case AttackKind::Leap: {
        const LeapStep step = StepLeap(attack, AttackerOf(boss), boss.leap, playerPosition, arena, dt);
        if (step.finished) {
            FinishAttack(boss, random);
        }
        return BossActions{.landed = step.landed};
    }
    case AttackKind::Volley: {
        const VolleyStep step = StepVolley(attack, AttackerOf(boss), boss.volley, player, arena, dt);
        if (step.finished) {
            FinishAttack(boss, random);
        }
        return BossActions{.spitVolley = step.spit};
    }
    case AttackKind::Summon: {
        const SummonStep step = StepSummon(attack, AttackerOf(boss), boss.summon, dt);
        if (step.finished) {
            FinishAttack(boss, random);
        }
        return BossActions{.summon = step.placeMarks};
    }
    }
    return {};
}

} // namespace

Boss MakeMatriarch(Vec2 position) {
    return Boss{.id = BossId::Matriarch, .motion = MotionAt(position)};
}

BossActions StepMatriarch(Boss& boss, const Motion& player, float playerRadius, const Arena& arena, Random& random,
                          float dt) {
    const Vec2 playerPosition = player.position;
    // Still and out of reach while it transforms; the attacks wait for the second phase.
    if (IsTransforming(boss)) {
        StepMovement(boss.motion, Vec2{}, boss.stats, dt);
        boss.transformSecondsLeft -= dt;
        return boss.transformSecondsLeft <= 0.0f ? EnterPhaseTwo(boss) : BossActions{};
    }
    StepShield(boss.shield, dt);
    if (IsStaggered(boss)) {
        boss.staggerSeconds -= dt;
        StepMovement(boss.motion, Vec2{}, boss.stats, dt);
        return {};
    }
    if (!boss.attack) {
        return BossActions{.attackStarted = Stalk(boss, playerPosition, playerRadius, arena, random, dt)};
    }
    BossActions actions = StepAttack(boss, player, arena, random, dt);
    // A combo skips its opener's recovery: the follow-up starts in the step the opener would begin to rest,
    // so there is no opening between the two. A Charge stunned by a wall shakes it off for the follow-up.
    if (boss.attack && boss.attack->clock.stage == AttackStage::Recovery && boss.attack->followUp) {
        StartAttack(boss, *boss.attack->followUp, true, playerPosition, arena, random);
        actions.attackStarted = true;
    }
    return actions;
}

bool IsTransforming(const Boss& boss) {
    return boss.phase == BossPhase::Transforming;
}

void StartTransformation(Boss& boss) {
    if (boss.phase != BossPhase::One) {
        return;
    }
    boss.phase = BossPhase::Transforming;
    boss.transformSecondsLeft = boss.phaseTwo.transformSeconds;
    boss.attack.reset();
    boss.staggerSeconds = 0.0f;
    boss.health.current = boss.health.max;
}

bool ShouldFrenzy(const Boss& boss) {
    const float threshold = boss.frenzy.healthFraction * static_cast<float>(boss.health.max);
    return boss.phase == BossPhase::Two && !boss.frenzied && !IsDead(boss.health) &&
           static_cast<float>(boss.health.current) <= threshold;
}

void EnterFrenzy(Boss& boss) {
    const FrenzyStats& frenzy = boss.frenzy;
    boss.frenzied = true;
    boss.choice = frenzy.choice;
    boss.combo = frenzy.combo;
    boss.charge.boltIntervalSeconds = frenzy.chargeBoltIntervalSeconds;
    boss.hairRing.waves = frenzy.ringWaves;
    boss.hairRing.gapTurnDegrees = frenzy.ringGapTurnDegrees;
    boss.leap.shadowSpeed = frenzy.leapShadowSpeed;
    boss.leap.landingStrands = frenzy.leapLandingStrands;
    boss.volley.bursts = frenzy.volleyBursts;
    boss.volley.burstIntervalSeconds = frenzy.volleyBurstIntervalSeconds;
    boss.summon.placement.count = frenzy.summonCount;
}

float SummonThreat(const Boss& boss, float distance) {
    const AttackChoiceStats& choice = boss.choice;
    if (IsDead(boss.health) || IsTransforming(boss) || choice.summon.farWeight <= 0.0f ||
        choice.summon.farWeight <= choice.summon.midWeight) {
        return 0.0f;
    }
    const float midDistance = 0.5f * (choice.nearDistance + choice.farDistance);
    if (choice.farDistance <= midDistance) {
        return distance >= choice.farDistance ? 1.0f : 0.0f;
    }
    const float t = std::clamp((distance - midDistance) / (choice.farDistance - midDistance), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

bool IsStaggered(const Boss& boss) {
    return boss.staggerSeconds > 0.0f;
}

void StaggerBoss(Boss& boss) {
    boss.attack.reset();
    boss.staggerSeconds = boss.breakStaggerSeconds;
    boss.secondsUntilAttack = boss.choice.minStalkSeconds;
}

} // namespace horde
