#include "core/attacks/Summon.h"

namespace horde {

float SummonWarningSeconds(const SummonStats& stats, const BossAttack& summon) {
    return stats.warningSeconds * summon.warningScale;
}

SummonStep StepSummon(BossAttack& summon, const Attacker& attacker, const SummonStats& stats, float dt) {
    StepMovement(attacker.motion, Vec2{}, attacker.walk, dt);
    TickClock(summon.clock, dt);
    SummonStep step;

    switch (summon.clock.stage) {
    case AttackStage::Warning:
        // `wavesSpat` counts the summons placed, so the marks go down once, on the first step of the howl.
        if (summon.wavesSpat == 0) {
            summon.wavesSpat = 1;
            step.placeMarks = true;
        }
        if (HasLasted(summon.clock, SummonWarningSeconds(stats, summon))) {
            EnterStage(summon.clock, AttackStage::Active);
        }
        break;
    case AttackStage::Active:
        if (HasLasted(summon.clock, stats.activeSeconds)) {
            EnterStage(summon.clock, AttackStage::Recovery);
        }
        break;
    case AttackStage::Recovery:
        step.finished = HasLasted(summon.clock, RecoverySeconds(attacker, summon, stats.recoverySeconds));
        break;
    }
    return step;
}

} // namespace horde
