#include "core/Boss.h"

#include <algorithm>
#include <limits>
#include <numbers>

#include "core/Chase.h"
#include "core/Collision.h"

namespace horde {

namespace {

constexpr float kRadiansPerDegree = std::numbers::pi_v<float> / 180.0f;

void EnterStage(BossAttack& attack, AttackStage stage) {
    attack.stage = stage;
    attack.stageSeconds = 0.0f;
}

// Toward the player, or the same way as before when the player stands right on the boss and gives no
// direction.
Vec2 AimAt(Vec2 from, Vec2 target, Vec2 previous) {
    const Vec2 offset = target - from;
    return LengthSquared(offset) > 1.0f ? Normalized(offset) : previous;
}

// Where the whole body fits inside the walls, so the boss never lands half inside one.
Vec2 InsideArena(Vec2 point, float radius, const Arena& arena) {
    return Vec2{std::clamp(point.x, arena.min.x + radius, arena.max.x - radius),
                std::clamp(point.y, arena.min.y + radius, arena.max.y - radius)};
}

// The second attack of a combo rests longer: the reward for getting through the whole chain.
float RecoverySeconds(const Boss& boss, const BossAttack& attack, float recovery) {
    return attack.endsCombo ? std::max(recovery, boss.combo.finalRecoverySeconds) : recovery;
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
        attack.takeoff = boss.motion.position;
        attack.target = InsideArena(playerPosition, boss.radius, arena);
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
void Stalk(Boss& boss, Vec2 playerPosition, float playerRadius, const Arena& arena, Random& random, float dt) {
    const float stopDistance = boss.radius + playerRadius + kContactSlack;
    StepMovement(boss.motion, ChaseInput(boss.motion.position, playerPosition, stopDistance), boss.stats, dt);

    boss.secondsUntilAttack -= dt;
    if (boss.secondsUntilAttack <= 0.0f) {
        const float distance = Length(playerPosition - boss.motion.position);
        const AttackKind kind =
            boss.nextAttack ? *boss.nextAttack : ChooseAttack(boss.choice, distance, boss.lastAttack, random);
        boss.nextAttack.reset();
        StartAttack(boss, kind, false, playerPosition, arena, random);
    }
}

void StepCharge(Boss& boss, BossAttack& charge, Vec2 playerPosition, const Arena& arena, Random& random, float dt) {
    const ChargeStats& stats = boss.charge;
    Motion& motion = boss.motion;
    charge.stageSeconds += dt;

    switch (charge.stage) {
    case AttackStage::Warning: {
        // It plants itself to crouch. The path follows the player until the aim locks, so standing still
        // or running away is not enough: the player has to move out of the line once it is fixed.
        StepMovement(motion, Vec2{}, boss.stats, dt);
        const float warning = WarningSeconds(boss, charge);
        if (charge.stageSeconds <= warning - stats.aimLockSeconds) {
            charge.direction = AimAt(motion.position, playerPosition, charge.direction);
        }
        if (charge.stageSeconds >= warning) {
            EnterStage(charge, AttackStage::Active);
            charge.distanceLeft = stats.distance;
        }
        break;
    }

    case AttackStage::Active: {
        // The run replaces normal movement, like the player's dash: a fixed speed in a fixed direction,
        // with no acceleration to read.
        motion.previousPosition = motion.position;
        const ChargePath path =
            PlanChargePath(motion.position, charge.direction, charge.distanceLeft, boss.radius, arena);

        const float step = stats.speed * dt;
        const float toEnd = Length(path.end - motion.position);
        if (step >= toEnd) {
            motion.position = path.end;
            motion.velocity = Vec2{};
            charge.stunned = path.endsAtWall;
            EnterStage(charge, AttackStage::Recovery);
        } else {
            motion.position += charge.direction * step;
            motion.velocity = charge.direction * stats.speed;
            charge.distanceLeft -= step;
        }
        break;
    }

    case AttackStage::Recovery: {
        StepMovement(motion, Vec2{}, boss.stats, dt);
        const float recovery = charge.stunned ? stats.stunSeconds : stats.recoverySeconds;
        if (charge.stageSeconds >= RecoverySeconds(boss, charge, recovery)) {
            FinishAttack(boss, random);
        }
        break;
    }
    }
}

// The boss holds its ground through the whole attack: it swells, spits and gets its breath back.
BossActions StepHairRing(Boss& boss, BossAttack& ring, Random& random, float dt) {
    const HairRingStats& stats = boss.hairRing;
    StepMovement(boss.motion, Vec2{}, boss.stats, dt);
    ring.stageSeconds += dt;
    BossActions actions;

    switch (ring.stage) {
    case AttackStage::Warning:
        if (ring.stageSeconds >= WarningSeconds(boss, ring)) {
            EnterStage(ring, AttackStage::Active);
        }
        break;

    case AttackStage::Active: {
        // Each ring leaves on its own beat, counted from the start of the stage. The gap turns before every
        // ring after the first, so that a few waves can sweep it around the boss.
        const float nextWave = static_cast<float>(ring.wavesSpat) * stats.waveIntervalSeconds;
        if (ring.wavesSpat < stats.waves && ring.stageSeconds >= nextWave) {
            if (ring.wavesSpat > 0) {
                ring.direction = Rotated(ring.direction, stats.gapTurnDegrees * kRadiansPerDegree);
            }
            ++ring.wavesSpat;
            actions.spitHairRing = true;
        }
        const float lastWave = static_cast<float>(stats.waves - 1) * stats.waveIntervalSeconds;
        if (ring.wavesSpat >= stats.waves && ring.stageSeconds >= lastWave + stats.activeSeconds) {
            EnterStage(ring, AttackStage::Recovery);
        }
        break;
    }

    case AttackStage::Recovery:
        if (ring.stageSeconds >= RecoverySeconds(boss, ring, stats.recoverySeconds)) {
            FinishAttack(boss, random);
        }
        break;
    }
    return actions;
}

BossActions StepLeap(Boss& boss, BossAttack& leap, Vec2 playerPosition, const Arena& arena, Random& random,
                     float dt) {
    const LeapStats& stats = boss.leap;
    Motion& motion = boss.motion;
    leap.stageSeconds += dt;
    BossActions actions;

    switch (leap.stage) {
    case AttackStage::Warning: {
        const float airSeconds = LeapAirSeconds(boss, leap);
        const float airborne = leap.stageSeconds - LeapCrouchSeconds(boss, leap);
        if (airborne <= 0.0f) {
            // Crouched, it plants itself, and the jump starts from where it comes to rest.
            StepMovement(motion, Vec2{}, boss.stats, dt);
            leap.takeoff = motion.position;
            break;
        }
        // The shadow chases the player until it locks, at a speed the player can outrun.
        if (airborne <= airSeconds - stats.lockSeconds) {
            const Vec2 chased = MoveTowards(leap.target, playerPosition, stats.shadowSpeed * dt);
            leap.target = InsideArena(chased, boss.radius, arena);
        }
        // The body travels through the air from the takeoff to the shadow, so the camera keeps both in view
        // and the drawing can arc it over the floor.
        motion.previousPosition = motion.position;
        motion.position = Lerp(leap.takeoff, leap.target, std::min(airborne / airSeconds, 1.0f));
        motion.velocity = Vec2{};
        if (airborne >= airSeconds) {
            motion.position = leap.target;
            EnterStage(leap, AttackStage::Active);
            actions.landed = true;
        }
        break;
    }

    case AttackStage::Active:
        StepMovement(motion, Vec2{}, boss.stats, dt);
        if (leap.stageSeconds >= stats.landingSeconds) {
            EnterStage(leap, AttackStage::Recovery);
        }
        break;

    case AttackStage::Recovery:
        StepMovement(motion, Vec2{}, boss.stats, dt);
        if (leap.stageSeconds >= RecoverySeconds(boss, leap, stats.recoverySeconds)) {
            FinishAttack(boss, random);
        }
        break;
    }
    return actions;
}

BossActions StepAttack(Boss& boss, Vec2 playerPosition, const Arena& arena, Random& random, float dt) {
    BossAttack& attack = *boss.attack;
    switch (attack.kind) {
    case AttackKind::Charge:
        StepCharge(boss, attack, playerPosition, arena, random, dt);
        return {};
    case AttackKind::HairRing:
        return StepHairRing(boss, attack, random, dt);
    case AttackKind::Leap:
        return StepLeap(boss, attack, playerPosition, arena, random, dt);
    }
    return {};
}

} // namespace

BossActions StepBoss(Boss& boss, Vec2 playerPosition, float playerRadius, const Arena& arena, Random& random,
                     float dt) {
    boss.hitFlashSeconds = std::max(boss.hitFlashSeconds - dt, 0.0f);

    // A dead boss drops whatever it was doing and only brakes to a stop.
    if (IsDead(boss.health)) {
        boss.attack.reset();
        StepMovement(boss.motion, Vec2{}, boss.stats, dt);
        return {};
    }
    StepShield(boss.shield, dt);
    if (IsStaggered(boss)) {
        boss.staggerSeconds -= dt;
        StepMovement(boss.motion, Vec2{}, boss.stats, dt);
        return {};
    }
    if (!boss.attack) {
        Stalk(boss, playerPosition, playerRadius, arena, random, dt);
        return {};
    }
    const BossActions actions = StepAttack(boss, playerPosition, arena, random, dt);
    // A combo skips its opener's recovery: the follow-up starts in the step the opener would begin to rest,
    // so there is no opening between the two. A Charge stunned by a wall shakes it off for the follow-up.
    if (boss.attack && boss.attack->stage == AttackStage::Recovery && boss.attack->followUp) {
        StartAttack(boss, *boss.attack->followUp, true, playerPosition, arena, random);
    }
    return actions;
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
    return boss.attack && boss.attack->kind == AttackKind::Charge && boss.attack->stage == AttackStage::Active;
}

bool IsAirborne(const Boss& boss) {
    const std::optional<BossAttack>& attack = boss.attack;
    return attack && attack->kind == AttackKind::Leap && attack->stage == AttackStage::Warning &&
           attack->stageSeconds > LeapCrouchSeconds(boss, *attack);
}

float WarningSeconds(const Boss& boss, const BossAttack& attack) {
    switch (attack.kind) {
    case AttackKind::Charge:
        return std::max(boss.charge.warningSeconds * attack.warningScale, boss.charge.aimLockSeconds);
    case AttackKind::HairRing:
        return boss.hairRing.warningSeconds * attack.warningScale;
    case AttackKind::Leap:
        return LeapCrouchSeconds(boss, attack) + LeapAirSeconds(boss, attack);
    }
    return 0.0f;
}

float LeapCrouchSeconds(const Boss& boss, const BossAttack& leap) {
    return boss.leap.crouchSeconds * leap.warningScale;
}

float LeapAirSeconds(const Boss& boss, const BossAttack& leap) {
    return std::max(boss.leap.airSeconds * leap.warningScale, boss.leap.lockSeconds);
}

ChargePath PlanChargePath(Vec2 start, Vec2 direction, float distance, float radius, const Arena& arena) {
    // How far the centre can travel before the body touches a wall, on each axis the path moves along.
    // The nearest of the two is where it hits.
    float toWall = std::numeric_limits<float>::infinity();
    const auto limit = [&toWall](float position, float speed, float low, float high) {
        if (speed > 0.0f) {
            toWall = std::min(toWall, (high - position) / speed);
        } else if (speed < 0.0f) {
            toWall = std::min(toWall, (low - position) / speed);
        }
    };
    limit(start.x, direction.x, arena.min.x + radius, arena.max.x - radius);
    limit(start.y, direction.y, arena.min.y + radius, arena.max.y - radius);
    toWall = std::max(toWall, 0.0f);

    if (toWall <= distance) {
        return ChargePath{.end = start + direction * toWall, .endsAtWall = true};
    }
    return ChargePath{.end = start + direction * distance, .endsAtWall = false};
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

} // namespace horde
