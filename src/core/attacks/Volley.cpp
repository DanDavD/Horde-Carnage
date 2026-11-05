#include "core/attacks/Volley.h"

#include <algorithm>

#include "core/DamageSource.h"

namespace horde {

float VolleyWarningSeconds(const VolleyStats& stats, const BossAttack& volley) {
    return stats.warningSeconds * volley.warningScale;
}

// The time of flight is guessed from the distance today, then again from the distance to the last guess. The
// strands are faster than the player, so each guess is closer than the one before, and a few are enough for
// a burst five strands wide. Strands leave the edge of the body, so that part of the distance is not flown.
// The lead is capped, and kept inside the walls the player cannot leave.
Vec2 LeadPoint(const VolleyStats& stats, Vec2 center, float radius, const Motion& player, const Arena& arena) {
    constexpr int kGuesses = 4;
    Vec2 ahead = player.position;
    for (int i = 0; i < kGuesses; ++i) {
        const float distance = std::max(Length(ahead - center) - radius, 0.0f);
        ahead = player.position + player.velocity * std::min(distance / stats.speed, stats.maxLeadSeconds);
    }
    return InsideArena(ahead, 0.0f, arena);
}

Projectile MakeVolleyStrand(const VolleyStats& stats, Vec2 center, float radius, Vec2 direction) {
    const Vec2 origin = center + direction * radius;
    Projectile strand = LaunchProjectile(origin, direction, stats.speed, stats.range);
    strand.radius = stats.strandRadius;
    strand.damage = stats.damage;
    strand.source = DamageSource::Volley;
    return strand;
}

VolleyStep StepVolley(BossAttack& volley, const Attacker& attacker, const VolleyStats& stats, const Motion& player,
                      const Arena& arena, float dt) {
    const Motion& body = attacker.motion;
    StepMovement(attacker.motion, Vec2{}, attacker.walk, dt);
    volley.direction = AimAt(body.position, player.position, volley.direction);
    TickClock(volley.clock, dt);
    VolleyStep step;

    switch (volley.clock.stage) {
    case AttackStage::Warning:
        if (HasLasted(volley.clock, VolleyWarningSeconds(stats, volley))) {
            EnterStage(volley.clock, AttackStage::Active);
        }
        break;

    case AttackStage::Active: {
        // Each burst on its own beat, counted from the start of the stage, like the waves of a ring.
        if (volley.wavesSpat < stats.bursts && IsBeatDue(volley.clock, volley.wavesSpat, stats.burstIntervalSeconds)) {
            if (volley.wavesSpat % 2 == 1) {
                const Vec2 lead = LeadPoint(stats, body.position, attacker.radius, player, arena);
                volley.direction = AimAt(body.position, lead, volley.direction);
            }
            ++volley.wavesSpat;
            step.spit = true;
        }
        if (volley.wavesSpat >= stats.bursts &&
            HasLastedPastBeats(volley.clock, stats.bursts, stats.burstIntervalSeconds, stats.activeSeconds)) {
            EnterStage(volley.clock, AttackStage::Recovery);
        }
        break;
    }

    case AttackStage::Recovery:
        step.finished = HasLasted(volley.clock, RecoverySeconds(attacker, volley, stats.recoverySeconds));
        break;
    }
    return step;
}

} // namespace horde
