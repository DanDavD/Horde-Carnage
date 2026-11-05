#pragma once

#include "core/BossAttack.h"
#include "core/SpawnMarker.h"

namespace horde {

/// The numbers of the Summon, the second phase's answer to a player who keeps far away.
struct SummonStats {
    float warningSeconds = 0.75f; ///< Howling, swollen, while the marks appear on the floor.
    float activeSeconds = 0.2f;
    float recoverySeconds = 1.0f; ///< Spent: the opening to punish it, if the hairballs allow.
    /// From the marks appearing to the hairballs coming out: as long as the warning, so the hairballs come
    /// out as the howl ends.
    float markerSeconds = 0.75f;
    /// Hairballs alive at once, at most. A summon brings fewer when the arena is already this full, so
    /// ignoring the hairballs does not bury the player.
    int maxAlive = 8;
    SummonPlacement placement; ///< Four marks 250 units around the player, inside the walls.
    /// Each summoned hairball spits a strand at the player this often, so the hairballs are something to
    /// watch, not only to outrun. Its first one comes at a random moment in the first two values, so a
    /// crowd does not spit in step.
    float spitIntervalSeconds = 3.0f;
    float firstSpitMinSeconds = 1.0f;
    float firstSpitMaxSeconds = 3.0f;
    float spitWarningSeconds = 0.4f; ///< It swells and its eyes redden for this long before it spits.
    /// A little faster than the Hair Ring's strands and far slower than the Volley's, and weaker than both,
    /// since there can be eight hairballs spitting.
    float spitSpeed = 300.0f;
    float spitRange = 700.0f;
    float spitRadius = 6.0f;
    int spitDamage = 8;
};

/// How long the warning of `summon` lasts: the stats' warning scaled by the attack's `warningScale`.
[[nodiscard]] float SummonWarningSeconds(const SummonStats& stats, const BossAttack& summon);

/// What one step of a Summon asks of the boss that runs it.
struct SummonStep {
    bool placeMarks = false; ///< Put the marks on the floor around the player now.
    bool finished = false;   ///< The recovery is over: the attacker goes back to stalking.
};

/// Advances a Summon by one step. The attacker holds its ground and howls; the marks go down as the howl
/// starts, and the world counts them down and lets the hairballs out, so the attacker is free to recover
/// while they come.
SummonStep StepSummon(BossAttack& summon, const Attacker& attacker, const SummonStats& stats, float dt);

} // namespace horde
