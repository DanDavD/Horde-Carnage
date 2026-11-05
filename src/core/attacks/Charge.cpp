#include "core/attacks/Charge.h"

#include <algorithm>
#include <limits>

namespace horde {

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

float ChargeWarningSeconds(const ChargeStats& stats, const BossAttack& charge) {
    return std::max(stats.warningSeconds * charge.warningScale, stats.aimLockSeconds);
}

ChargeStep StepCharge(BossAttack& charge, const Attacker& attacker, const ChargeStats& stats, Vec2 playerPosition,
                      const Arena& arena, float dt) {
    Motion& motion = attacker.motion;
    TickClock(charge.clock, dt);
    ChargeStep step;

    switch (charge.clock.stage) {
    case AttackStage::Warning: {
        // It plants itself to crouch. The path follows the player until the aim locks, so standing still
        // or running away is not enough: the player has to move out of the line once it is fixed.
        StepMovement(motion, Vec2{}, attacker.walk, dt);
        const float warning = ChargeWarningSeconds(stats, charge);
        if (charge.clock.seconds <= warning - stats.aimLockSeconds) {
            charge.direction = AimAt(motion.position, playerPosition, charge.direction);
        }
        if (HasLasted(charge.clock, warning)) {
            EnterStage(charge.clock, AttackStage::Active);
            charge.distanceLeft = stats.distance;
        }
        break;
    }

    case AttackStage::Active: {
        // In a frenzy it throws strands as it runs, on a beat counted from the start of the run.
        if (stats.boltIntervalSeconds > 0.0f && IsBeatDue(charge.clock, charge.wavesSpat, stats.boltIntervalSeconds)) {
            ++charge.wavesSpat;
            step.throwBolts = true;
        }
        // The run replaces normal movement, like the player's dash: a fixed speed in a fixed direction,
        // with no acceleration to read.
        motion.previousPosition = motion.position;
        const ChargePath path =
            PlanChargePath(motion.position, charge.direction, charge.distanceLeft, attacker.radius, arena);

        const float run = stats.speed * dt;
        const float toEnd = Length(path.end - motion.position);
        if (run >= toEnd) {
            motion.position = path.end;
            motion.velocity = Vec2{};
            charge.stunned = path.endsAtWall;
            step.hitWall = path.endsAtWall;
            EnterStage(charge.clock, AttackStage::Recovery);
        } else {
            motion.position += charge.direction * run;
            motion.velocity = charge.direction * stats.speed;
            charge.distanceLeft -= run;
        }
        break;
    }

    case AttackStage::Recovery: {
        StepMovement(motion, Vec2{}, attacker.walk, dt);
        const float recovery = charge.stunned ? stats.stunSeconds : stats.recoverySeconds;
        step.finished = HasLasted(charge.clock, RecoverySeconds(attacker, charge, recovery));
        break;
    }
    }
    return step;
}

} // namespace horde
