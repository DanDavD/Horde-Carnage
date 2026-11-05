#include "core/Boss.h"

#include <algorithm>
#include <numbers>

#include "core/Chase.h"
#include "core/Collision.h"

namespace horde {

namespace {

constexpr float kRadiansPerDegree = std::numbers::pi_v<float> / 180.0f;

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
    case AttackKind::HairRing: {
        // To one side of the player, and to the other side the next time, so reaching the gap always means
        // moving, and the player cannot settle into one habit.
        const float side = boss.ringsStarted % 2 == 0 ? 1.0f : -1.0f;
        attack.direction = Rotated(toPlayer, side * boss.hairRing.gapAngleDegrees * kRadiansPerDegree);
        ++boss.ringsStarted;
        break;
    }
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

// The boss holds its ground through the whole attack: it swells, spits and gets its breath back.
BossActions StepHairRing(Boss& boss, BossAttack& ring, Random& random, float dt) {
    const HairRingStats& stats = boss.hairRing;
    StepMovement(boss.motion, Vec2{}, boss.stats, dt);
    TickClock(ring.clock, dt);
    BossActions actions;

    switch (ring.clock.stage) {
    case AttackStage::Warning:
        if (HasLasted(ring.clock, WarningSeconds(boss, ring))) {
            EnterStage(ring.clock, AttackStage::Active);
        }
        break;

    case AttackStage::Active: {
        // Each ring leaves on its own beat, counted from the start of the stage. The gap turns before every
        // ring after the first, so that a few waves can sweep it around the boss.
        if (ring.wavesSpat < stats.waves && IsBeatDue(ring.clock, ring.wavesSpat, stats.waveIntervalSeconds)) {
            if (ring.wavesSpat > 0) {
                ring.direction = Rotated(ring.direction, stats.gapTurnDegrees * kRadiansPerDegree);
            }
            ++ring.wavesSpat;
            actions.spitHairRing = true;
        }
        if (ring.wavesSpat >= stats.waves &&
            HasLastedPastBeats(ring.clock, stats.waves, stats.waveIntervalSeconds, stats.activeSeconds)) {
            EnterStage(ring.clock, AttackStage::Recovery);
        }
        break;
    }

    case AttackStage::Recovery:
        if (HasLasted(ring.clock, RecoverySeconds(AttackerOf(boss), ring, stats.recoverySeconds))) {
            FinishAttack(boss, random);
        }
        break;
    }
    return actions;
}

// The boss holds its ground and keeps turning to the player. Even bursts (the first and the third) leave
// toward where the player is; odd ones toward where they will be.
BossActions StepVolley(Boss& boss, BossAttack& volley, const Motion& player, const Arena& arena, Random& random,
                       float dt) {
    const VolleyStats& stats = boss.volley;
    StepMovement(boss.motion, Vec2{}, boss.stats, dt);
    volley.direction = AimAt(boss.motion.position, player.position, volley.direction);
    TickClock(volley.clock, dt);
    BossActions actions;

    switch (volley.clock.stage) {
    case AttackStage::Warning:
        if (HasLasted(volley.clock, WarningSeconds(boss, volley))) {
            EnterStage(volley.clock, AttackStage::Active);
        }
        break;

    case AttackStage::Active: {
        // Each burst on its own beat, counted from the start of the stage, like the waves of a ring.
        if (volley.wavesSpat < stats.bursts && IsBeatDue(volley.clock, volley.wavesSpat, stats.burstIntervalSeconds)) {
            if (volley.wavesSpat % 2 == 1) {
                volley.direction = AimAt(boss.motion.position, LeadPoint(boss, player, arena), volley.direction);
            }
            ++volley.wavesSpat;
            actions.spitVolley = true;
        }
        if (volley.wavesSpat >= stats.bursts &&
            HasLastedPastBeats(volley.clock, stats.bursts, stats.burstIntervalSeconds, stats.activeSeconds)) {
            EnterStage(volley.clock, AttackStage::Recovery);
        }
        break;
    }

    case AttackStage::Recovery:
        if (HasLasted(volley.clock, RecoverySeconds(AttackerOf(boss), volley, stats.recoverySeconds))) {
            FinishAttack(boss, random);
        }
        break;
    }
    return actions;
}

// The boss holds its ground and howls. The marks go down as the howl starts; the world counts them down and
// lets the hairballs out, so the boss is free to recover while they come.
BossActions StepSummon(Boss& boss, BossAttack& summon, Random& random, float dt) {
    const SummonStats& stats = boss.summon;
    StepMovement(boss.motion, Vec2{}, boss.stats, dt);
    TickClock(summon.clock, dt);
    BossActions actions;

    switch (summon.clock.stage) {
    case AttackStage::Warning:
        if (summon.wavesSpat == 0) {
            summon.wavesSpat = 1;
            actions.summon = true;
        }
        if (HasLasted(summon.clock, WarningSeconds(boss, summon))) {
            EnterStage(summon.clock, AttackStage::Active);
        }
        break;
    case AttackStage::Active:
        if (HasLasted(summon.clock, stats.activeSeconds)) {
            EnterStage(summon.clock, AttackStage::Recovery);
        }
        break;
    case AttackStage::Recovery:
        if (HasLasted(summon.clock, RecoverySeconds(AttackerOf(boss), summon, stats.recoverySeconds))) {
            FinishAttack(boss, random);
        }
        break;
    }
    return actions;
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
    case AttackKind::HairRing:
        return StepHairRing(boss, attack, random, dt);
    case AttackKind::Leap: {
        const LeapStep step = StepLeap(attack, AttackerOf(boss), boss.leap, playerPosition, arena, dt);
        if (step.finished) {
            FinishAttack(boss, random);
        }
        return BossActions{.landed = step.landed};
    }
    case AttackKind::Volley:
        return StepVolley(boss, attack, player, arena, random, dt);
    case AttackKind::Summon:
        return StepSummon(boss, attack, random, dt);
    }
    return {};
}

} // namespace

BossActions StepBoss(Boss& boss, const Motion& player, float playerRadius, const Arena& arena, Random& random,
                     float dt) {
    const Vec2 playerPosition = player.position;
    boss.hitFlashSeconds = std::max(boss.hitFlashSeconds - dt, 0.0f);

    // A dead boss drops whatever it was doing and only brakes to a stop.
    if (IsDead(boss.health)) {
        boss.attack.reset();
        StepMovement(boss.motion, Vec2{}, boss.stats, dt);
        return {};
    }
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
        return boss.hairRing.warningSeconds * attack.warningScale;
    case AttackKind::Leap:
        return LeapCrouchSeconds(boss, attack) + LeapAirSeconds(boss, attack);
    case AttackKind::Volley:
        return boss.volley.warningSeconds * attack.warningScale;
    case AttackKind::Summon:
        return boss.summon.warningSeconds * attack.warningScale;
    }
    return 0.0f;
}

float LeapCrouchSeconds(const Boss& boss, const BossAttack& leap) {
    return LeapCrouchSeconds(boss.leap, leap);
}

float LeapAirSeconds(const Boss& boss, const BossAttack& leap) {
    return LeapAirSeconds(boss.leap, leap);
}

int HairRingStrands(const HairRingStats& stats) {
    return std::max(stats.slots - stats.gapSlots, 0);
}

Vec2 HairStrandDirection(Vec2 gap, int index, const HairRingStats& stats) {
    // The empty slots sit evenly around the middle of the gap, so the strands on its two edges are
    // (gapSlots + 1) / 2 slots away from the middle, one on each side. The rest go round from one to the
    // other.
    const float slot = 2.0f * std::numbers::pi_v<float> / static_cast<float>(stats.slots);
    const float firstStrand = 0.5f * static_cast<float>(stats.gapSlots + 1) * slot;
    return Rotated(gap, firstStrand + static_cast<float>(index) * slot);
}

Projectile MakeHairStrand(const Boss& boss, Vec2 direction) {
    const HairRingStats& stats = boss.hairRing;
    // From the edge of the body, so a player pressed against the boss is caught at once.
    const Vec2 origin = boss.motion.position + direction * boss.radius;
    Projectile strand = LaunchProjectile(origin, direction, stats.speed, stats.range);
    strand.radius = stats.strandRadius;
    strand.damage = stats.damage;
    return strand;
}

// Where a strand leaving now meets a player who keeps their velocity. The time of flight is guessed from the
// distance today, then again from the distance to the last guess. The strands are faster than the player, so
// each guess is closer than the one before, and a few are enough for a burst three strands wide. Strands
// leave the edge of the body, so that part of the distance is not flown. The lead is capped, and kept inside
// the walls the player cannot leave.
Vec2 LeadPoint(const Boss& boss, const Motion& player, const Arena& arena) {
    constexpr int kGuesses = 4;
    const VolleyStats& stats = boss.volley;
    Vec2 ahead = player.position;
    for (int i = 0; i < kGuesses; ++i) {
        const float distance = std::max(Length(ahead - boss.motion.position) - boss.radius, 0.0f);
        ahead = player.position + player.velocity * std::min(distance / stats.speed, stats.maxLeadSeconds);
    }
    return InsideArena(ahead, 0.0f, arena);
}

Projectile MakeVolleyStrand(const Boss& boss, Vec2 direction) {
    const VolleyStats& stats = boss.volley;
    const Vec2 origin = boss.motion.position + direction * boss.radius;
    Projectile strand = LaunchProjectile(origin, direction, stats.speed, stats.range);
    strand.radius = stats.strandRadius;
    strand.damage = stats.damage;
    strand.source = DamageSource::Volley;
    return strand;
}

} // namespace horde
