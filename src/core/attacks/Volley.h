#pragma once

#include "core/Arena.h"
#include "core/BossAttack.h"
#include "core/Movement.h"
#include "core/Projectile.h"
#include "core/Vec2.h"

namespace horde {

/// The numbers of the Aimed Volley, the second phase's ranged attack.
struct VolleyStats {
    float warningSeconds = 0.55f; ///< Leaning back, with a line on the floor toward the player.
    int bursts = 3;
    /// Between two bursts. The bursts take turns: the first goes where the player is, the second where
    /// the player will be if they keep running, the third where they are again. Running in a straight line
    /// is caught by the second, stopping by the first and third: only a change of rhythm gets through.
    float burstIntervalSeconds = 0.3f;
    /// How far ahead a led burst looks, at most. Across the whole arena the flight is longer than this, and
    /// leading a player by all of it would aim at empty floor.
    float maxLeadSeconds = 1.5f;
    float activeSeconds = 0.15f;  ///< After the last burst, before the recovery.
    float recoverySeconds = 0.8f; ///< Spent: the opening to punish it.
    /// A fan 32 degrees wide: a step to the side is not enough to leave it, close up. Far away the gaps
    /// between the strands open up enough to slip through.
    int strandsPerBurst = 5;
    float spreadDegrees = 8.0f; ///< Between two strands of a burst.
    /// Units per second. More than twice the Hair Ring's: across the arena, 600 units, it still takes over a
    /// second to arrive, so it can be read, but standing far away is no longer enough on its own.
    float speed = 520.0f;
    float range = 900.0f;
    float strandRadius = 7.0f;
    int damage = 12; ///< Per strand. A burst lands at most one hit, through the invulnerability window.
};

/// How long the warning of `volley` lasts: the stats' warning scaled by the attack's `warningScale`.
[[nodiscard]] float VolleyWarningSeconds(const VolleyStats& stats, const BossAttack& volley);

/// Where a strand leaving the edge of a body of `radius` at `center` now meets a player who keeps their
/// velocity, at most `maxLeadSeconds` ahead and inside the arena.
[[nodiscard]] Vec2 LeadPoint(const VolleyStats& stats, Vec2 center, float radius, const Motion& player,
                             const Arena& arena);

/// One strand of a burst spat by a body of `radius` at `center`, leaving its edge in `direction` (length 1).
[[nodiscard]] Projectile MakeVolleyStrand(const VolleyStats& stats, Vec2 center, float radius, Vec2 direction);

/// What one step of a Volley asks of the boss that runs it.
struct VolleyStep {
    bool spit = false;     ///< Spawn one burst now, aimed along `BossAttack::direction`.
    bool finished = false; ///< The recovery is over: the attacker goes back to stalking.
};

/// Advances a Volley by one step. The attacker holds its ground and keeps turning to the player; its bursts
/// leave on their beats, the even ones toward where the player is and the odd ones toward where they will be.
VolleyStep StepVolley(BossAttack& volley, const Attacker& attacker, const VolleyStats& stats, const Motion& player,
                      const Arena& arena, float dt);

} // namespace horde
