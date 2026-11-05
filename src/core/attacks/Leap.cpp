#include "core/attacks/Leap.h"

#include <algorithm>

namespace horde {

void AimLeap(BossAttack& leap, Vec2 takeoff, Vec2 playerPosition, float radius, const Arena& arena) {
    leap.takeoff = takeoff;
    leap.target = InsideArena(playerPosition, radius, arena);
}

float LeapCrouchSeconds(const LeapStats& stats, const BossAttack& leap) {
    return stats.crouchSeconds * leap.warningScale;
}

float LeapAirSeconds(const LeapStats& stats, const BossAttack& leap) {
    return std::max(stats.airSeconds * leap.warningScale, stats.lockSeconds);
}

bool IsLeapAirborne(const LeapStats& stats, const BossAttack& leap) {
    return leap.clock.stage == AttackStage::Warning && leap.clock.seconds > LeapCrouchSeconds(stats, leap);
}

LeapStep StepLeap(BossAttack& leap, const Attacker& attacker, const LeapStats& stats, Vec2 playerPosition,
                  const Arena& arena, float dt) {
    Motion& motion = attacker.motion;
    TickClock(leap.clock, dt);
    LeapStep step;

    switch (leap.clock.stage) {
    case AttackStage::Warning: {
        const float airSeconds = LeapAirSeconds(stats, leap);
        const float airborne = leap.clock.seconds - LeapCrouchSeconds(stats, leap);
        if (airborne <= 0.0f) {
            // Crouched, it plants itself, and the jump starts from where it comes to rest.
            StepMovement(motion, Vec2{}, attacker.walk, dt);
            leap.takeoff = motion.position;
            break;
        }
        // The shadow chases the player until it locks, at a speed the player can outrun.
        if (airborne <= airSeconds - stats.lockSeconds) {
            const Vec2 chased = MoveTowards(leap.target, playerPosition, stats.shadowSpeed * dt);
            leap.target = InsideArena(chased, attacker.radius, arena);
        }
        // The body travels through the air from the takeoff to the shadow, so the camera keeps both in view
        // and the drawing can arc it over the floor.
        motion.previousPosition = motion.position;
        motion.position = Lerp(leap.takeoff, leap.target, std::min(airborne / airSeconds, 1.0f));
        motion.velocity = Vec2{};
        if (airborne >= airSeconds) {
            motion.position = leap.target;
            EnterStage(leap.clock, AttackStage::Active);
            step.landed = true;
        }
        break;
    }

    case AttackStage::Active:
        StepMovement(motion, Vec2{}, attacker.walk, dt);
        if (HasLasted(leap.clock, stats.landingSeconds)) {
            EnterStage(leap.clock, AttackStage::Recovery);
        }
        break;

    case AttackStage::Recovery:
        StepMovement(motion, Vec2{}, attacker.walk, dt);
        step.finished = HasLasted(leap.clock, RecoverySeconds(attacker, leap, stats.recoverySeconds));
        break;
    }
    return step;
}

} // namespace horde
