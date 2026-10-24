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

// For now the attacks take turns. On day 7 a seeded random choice takes over.
void StartAttack(Boss& boss, Vec2 playerPosition) {
    const Vec2 toPlayer = AimAt(boss.motion.position, playerPosition, Vec2{0.0f, 1.0f});
    BossAttack attack{.kind = boss.nextAttack, .direction = toPlayer};
    switch (boss.nextAttack) {
    case AttackKind::Charge:
        boss.nextAttack = AttackKind::HairRing;
        break;
    case AttackKind::HairRing: {
        // To one side of the player, and to the other side the next time, so reaching the gap always means
        // moving, and the player cannot settle into one habit.
        const float side = boss.ringsStarted % 2 == 0 ? 1.0f : -1.0f;
        attack.direction = Rotated(toPlayer, side * boss.hairRing.gapAngleDegrees * kRadiansPerDegree);
        ++boss.ringsStarted;
        boss.nextAttack = AttackKind::Charge;
        break;
    }
    }
    boss.attack = attack;
}

// Back to stalking, with the whole interval ahead.
void FinishAttack(Boss& boss) {
    boss.attack.reset();
    boss.secondsUntilAttack = boss.attackInterval;
}

// Walks at the player, like a big, slow hairball, until it is time to attack.
void Stalk(Boss& boss, Vec2 playerPosition, float playerRadius, float dt) {
    const float stopDistance = boss.radius + playerRadius + kContactSlack;
    StepMovement(boss.motion, ChaseInput(boss.motion.position, playerPosition, stopDistance), boss.stats, dt);

    boss.secondsUntilAttack -= dt;
    if (boss.secondsUntilAttack <= 0.0f) {
        StartAttack(boss, playerPosition);
    }
}

void StepCharge(Boss& boss, BossAttack& charge, Vec2 playerPosition, const Arena& arena, float dt) {
    const ChargeStats& stats = boss.charge;
    Motion& motion = boss.motion;
    charge.stageSeconds += dt;

    switch (charge.stage) {
    case AttackStage::Warning:
        // It plants itself to crouch. The path follows the player until the aim locks, so standing still
        // or running away is not enough: the player has to move out of the line once it is fixed.
        StepMovement(motion, Vec2{}, boss.stats, dt);
        if (charge.stageSeconds <= stats.warningSeconds - stats.aimLockSeconds) {
            charge.direction = AimAt(motion.position, playerPosition, charge.direction);
        }
        if (charge.stageSeconds >= stats.warningSeconds) {
            EnterStage(charge, AttackStage::Active);
            charge.distanceLeft = stats.distance;
        }
        break;

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
        if (charge.stageSeconds >= recovery) {
            FinishAttack(boss);
        }
        break;
    }
    }
}

// The boss holds its ground through the whole attack: it swells, spits and gets its breath back.
BossActions StepHairRing(Boss& boss, BossAttack& ring, float dt) {
    const HairRingStats& stats = boss.hairRing;
    StepMovement(boss.motion, Vec2{}, boss.stats, dt);
    ring.stageSeconds += dt;
    BossActions actions;

    switch (ring.stage) {
    case AttackStage::Warning:
        if (ring.stageSeconds >= stats.warningSeconds) {
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
        if (ring.stageSeconds >= stats.recoverySeconds) {
            FinishAttack(boss);
        }
        break;
    }
    return actions;
}

} // namespace

BossActions StepBoss(Boss& boss, Vec2 playerPosition, float playerRadius, const Arena& arena, float dt) {
    boss.hitFlashSeconds = std::max(boss.hitFlashSeconds - dt, 0.0f);

    // A dead boss drops whatever it was doing and only brakes to a stop.
    if (IsDead(boss.health)) {
        boss.attack.reset();
        StepMovement(boss.motion, Vec2{}, boss.stats, dt);
        return {};
    }
    if (!boss.attack) {
        Stalk(boss, playerPosition, playerRadius, dt);
        return {};
    }
    switch (boss.attack->kind) {
    case AttackKind::Charge:
        StepCharge(boss, *boss.attack, playerPosition, arena, dt);
        return {};
    case AttackKind::HairRing:
        return StepHairRing(boss, *boss.attack, dt);
    }
    return {};
}

bool IsCharging(const Boss& boss) {
    return boss.attack && boss.attack->kind == AttackKind::Charge && boss.attack->stage == AttackStage::Active;
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
