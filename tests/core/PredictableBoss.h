#pragma once

#include "core/Boss.h"

namespace horde::test {

/// A boss without its chance: every warning at its stats' exact length and no combos. The tests about one
/// attack's timings use it, so they do not depend on what the generator draws.
inline Boss Predictable(Boss boss) {
    boss.choice.minWarningScale = 1.0f;
    boss.choice.maxWarningScale = 1.0f;
    boss.combo.chance = 0.0f;
    return boss;
}

} // namespace horde::test
