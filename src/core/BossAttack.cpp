#include "core/BossAttack.h"

#include <algorithm>

namespace horde {

float RecoverySeconds(const Attacker& attacker, const BossAttack& attack, float recovery) {
    return attack.endsCombo ? std::max(recovery, attacker.comboRecoverySeconds) : recovery;
}

Vec2 AimAt(Vec2 from, Vec2 target, Vec2 previous) {
    const Vec2 offset = target - from;
    return LengthSquared(offset) > 1.0f ? Normalized(offset) : previous;
}

} // namespace horde
