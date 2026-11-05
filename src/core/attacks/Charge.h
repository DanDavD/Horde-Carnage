#pragma once

#include "core/Arena.h"
#include "core/BossAttack.h"
#include "core/Vec2.h"

namespace horde {

/// The numbers of the Charge.
struct ChargeStats {
    float warningSeconds = 0.8f; ///< Crouched, with its path drawn on the floor.
    /// The last part of the warning, in which the path stops following the player. That is the moment to
    /// get out of the way: dodging earlier only drags the path along.
    float aimLockSeconds = 0.3f;
    float speed = 1000.0f; ///< Units per second. Faster than the player, slower than a dash.
    /// The run overshoots the player by a lot, so it usually ends at a wall. From the open middle of the
    /// arena it can stop short of one.
    float distance = 800.0f;
    int damage = 30;          ///< Instead of the touch damage, while it runs.
    float knockback = 700.0f; ///< Units per second added to the player, sideways out of the path.
    float recoverySeconds = 0.5f; ///< After a run that ends in the open.
    float stunSeconds = 1.5f;     ///< After running into a wall: the opening to punish it.
    /// In a frenzy only: while it runs it throws a pair of strands this often, one at the player and one
    /// where the player is going. 0 throws none.
    float boltIntervalSeconds = 0.0f;
};

/// Where a charge would end: after its full distance, or earlier, where the body meets a wall.
struct ChargePath {
    Vec2 end;
    bool endsAtWall = false;
};

/// The path of a charge from `start` along `direction` (length 1), for a body of `radius`: the full
/// `distance`, or less if a wall comes first. The simulation runs along it and the warning draws it, so
/// what the player sees is what will happen.
[[nodiscard]] ChargePath PlanChargePath(Vec2 start, Vec2 direction, float distance, float radius, const Arena& arena);

/// How long the warning of `charge` lasts: the stats' warning scaled by the attack's `warningScale`, never
/// shorter than the aim lock at its end.
[[nodiscard]] float ChargeWarningSeconds(const ChargeStats& stats, const BossAttack& charge);

/// What one step of a Charge asks of the boss that runs it.
struct ChargeStep {
    bool throwBolts = false; ///< In a frenzied run: throw a pair of strands now.
    bool hitWall = false;    ///< The run just ended against a wall, and the attacker is stunned.
    bool finished = false;   ///< The recovery is over: the attacker goes back to stalking.
};

/// Advances a Charge by one step. Crouched, the attacker holds its ground and its aim follows the player
/// until it locks; then it runs at a fixed speed along `PlanChargePath`; then it recovers, for longer if the
/// run ended at a wall. It moves `attacker.motion`.
ChargeStep StepCharge(BossAttack& charge, const Attacker& attacker, const ChargeStats& stats, Vec2 playerPosition,
                      const Arena& arena, float dt);

} // namespace horde
