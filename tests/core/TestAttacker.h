#pragma once

#include "core/BossAttack.h"
#include "core/Movement.h"

namespace horde::test {

/// A sixty-fourth of a second is exact in binary, so every stage in the attack tests lasts a whole number of
/// steps and times can be compared with ==.
inline constexpr float kAttackDt = 1.0f / 64.0f;

/// A body that is not the Matriarch's, for running the attacks in core/attacks/ as a second boss would.
struct TestBody {
    Motion motion;
    MovementStats walk;
    float radius = 30.0f;

    [[nodiscard]] Attacker AsAttacker() { return Attacker{.motion = motion, .walk = walk, .radius = radius}; }
};

/// A body at rest at `position`.
inline TestBody BodyAt(Vec2 position) {
    return TestBody{.motion = MotionAt(position)};
}

} // namespace horde::test
