#include "core/attacks/HairRing.h"

#include <algorithm>
#include <numbers>

namespace horde {

namespace {

constexpr float kRadiansPerDegree = std::numbers::pi_v<float> / 180.0f;

} // namespace

void AimHairRing(BossAttack& ring, Vec2 toPlayer, bool otherSide, const HairRingStats& stats) {
    const float side = otherSide ? -1.0f : 1.0f;
    ring.direction = Rotated(toPlayer, side * stats.gapAngleDegrees * kRadiansPerDegree);
}

float HairRingWarningSeconds(const HairRingStats& stats, const BossAttack& ring) {
    return stats.warningSeconds * ring.warningScale;
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

Projectile MakeHairStrand(const HairRingStats& stats, Vec2 center, float radius, Vec2 direction) {
    // From the edge of the body, so a player pressed against the boss is caught at once.
    const Vec2 origin = center + direction * radius;
    Projectile strand = LaunchProjectile(origin, direction, stats.speed, stats.range);
    strand.radius = stats.strandRadius;
    strand.damage = stats.damage;
    return strand;
}

HairRingStep StepHairRing(BossAttack& ring, const Attacker& attacker, const HairRingStats& stats, float dt) {
    StepMovement(attacker.motion, Vec2{}, attacker.walk, dt);
    TickClock(ring.clock, dt);
    HairRingStep step;

    switch (ring.clock.stage) {
    case AttackStage::Warning:
        if (HasLasted(ring.clock, HairRingWarningSeconds(stats, ring))) {
            EnterStage(ring.clock, AttackStage::Active);
        }
        break;

    case AttackStage::Active: {
        // Each ring leaves on its own beat, counted from the start of the stage. The gap turns before every
        // ring after the first, so that a few waves can sweep it around the attacker.
        if (ring.wavesSpat < stats.waves && IsBeatDue(ring.clock, ring.wavesSpat, stats.waveIntervalSeconds)) {
            if (ring.wavesSpat > 0) {
                ring.direction = Rotated(ring.direction, stats.gapTurnDegrees * kRadiansPerDegree);
            }
            ++ring.wavesSpat;
            step.spit = true;
        }
        if (ring.wavesSpat >= stats.waves &&
            HasLastedPastBeats(ring.clock, stats.waves, stats.waveIntervalSeconds, stats.activeSeconds)) {
            EnterStage(ring.clock, AttackStage::Recovery);
        }
        break;
    }

    case AttackStage::Recovery:
        step.finished = HasLasted(ring.clock, RecoverySeconds(attacker, ring, stats.recoverySeconds));
        break;
    }
    return step;
}

} // namespace horde
