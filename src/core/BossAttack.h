#pragma once

#include <optional>

#include "core/AttackChoice.h"
#include "core/AttackClock.h"
#include "core/Movement.h"
#include "core/Vec2.h"

namespace horde {

/// The attack in progress. Every attack keeps its state here, whichever boss runs it.
struct BossAttack {
    AttackKind kind = AttackKind::Charge;
    AttackClock clock; ///< Its stage, and the time spent in it.
    /// Where the attack points, length 1. Charge: where it runs; it follows the player through the warning
    /// until the aim locks. Hair Ring: the middle of the gap, fixed from the start of the warning.
    Vec2 direction{0.0f, 1.0f};
    float distanceLeft = 0.0f; ///< Charge: how much of the run is left while it is active.
    bool stunned = false;      ///< Charge: it ran into a wall, so the recovery is the long one.
    int wavesSpat = 0;         ///< Hair Ring: how many of its rings it has spat so far.
    Vec2 takeoff; ///< Leap: where it left the ground.
    /// Leap: where it will land, drawn as its shadow. It follows the player through the air until it locks.
    Vec2 target;
    /// How much longer or shorter than its stats the warning lasts, picked when the attack starts. Read it
    /// through `WarningSeconds`.
    float warningScale = 1.0f;
    /// The attack that starts the moment this one would begin its recovery: set when this attack opens a
    /// combo.
    std::optional<AttackKind> followUp;
    bool endsCombo = false; ///< The second attack of a combo: a shorter warning and a longer recovery.
};

/// The body that carries out an attack, and the one rule of the boss's that every attack follows.
///
/// The attacks in `core/attacks/` take one of these, with their own numbers, instead of a whole `Boss`. That
/// is what lets a second boss run a Charge or a Leap with a different body, speed or timing. The attack moves
/// `motion`, which belongs to the boss.
struct Attacker {
    Motion& motion;
    const MovementStats& walk; ///< How it brakes while it holds its ground.
    float radius = 0.0f;
    /// The shortest recovery after the second attack of a combo: the reward for getting through the chain.
    float comboRecoverySeconds = 0.0f;
};

/// How long `attack` recovers: `recovery`, or the attacker's `comboRecoverySeconds` if the attack ends a combo
/// and that is longer.
[[nodiscard]] float RecoverySeconds(const Attacker& attacker, const BossAttack& attack, float recovery);

/// The direction from `from` toward `target`, length 1, or `previous` when the target is right on top of it
/// and gives no direction.
[[nodiscard]] Vec2 AimAt(Vec2 from, Vec2 target, Vec2 previous);

} // namespace horde
