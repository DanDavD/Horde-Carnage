#pragma once

#include "core/AttackChoice.h"
#include "core/Shockwave.h"

// The numbers only the Matriarch has: her second phase and her frenzy. The Boss struct carries them, so
// they live in a header of their own that Boss.h includes; her rules are in bosses/Matriarch.h.

namespace horde {

/// The second phase's weights: the Volley and the Summon join the three attacks of the first. Both are at
/// their best far away, where the others leave gaps, so keeping away is no longer the safe answer. Stalks
/// are shorter and warnings a little shorter on average.
inline constexpr AttackChoiceStats PhaseTwoChoice() {
    AttackChoiceStats choice;
    choice.volley = AttackWeights{.nearWeight = 1.0f, .midWeight = 3.0f, .farWeight = 3.0f};
    choice.summon = AttackWeights{.nearWeight = 0.5f, .midWeight = 1.0f, .farWeight = 2.5f};
    choice.minStalkSeconds = 0.6f;
    choice.maxStalkSeconds = 1.2f;
    choice.minWarningScale = 0.75f;
    choice.maxWarningScale = 1.0f;
    return choice;
}

/// What changes when the boss enters its second phase. It keeps its attacks and its body, and these numbers
/// replace the first phase's.
struct PhaseTwoStats {
    /// Invulnerable and roaring, with its bar refilling, between the two phases.
    float transformSeconds = 2.0f;
    AttackChoiceStats choice = PhaseTwoChoice();
    /// Half of the attacks that can open a combo do, with a shorter rest after the chain.
    ComboStats combo{.chance = 0.5f, .warningScale = 0.7f, .finalRecoverySeconds = 1.0f};
    float exposedSeconds = 4.0f; ///< Of each broken shield: a shorter opening.
    /// The roar that ends the transformation sends a shockwave like a landing's, but shorter, so it punishes
    /// a player who waits next to the boss for phase 2, not one who keeps their distance (the fight starts
    /// 400 units apart).
    ShockwaveStats roar{.speed = 380.0f, .reach = 300.0f, .thickness = 20.0f, .damage = 20};
};

/// The second phase's stalks and combos once the boss is in a frenzy.
inline constexpr AttackChoiceStats FrenzyChoice() {
    AttackChoiceStats choice = PhaseTwoChoice();
    choice.minStalkSeconds = 0.4f;
    choice.maxStalkSeconds = 0.9f;
    return choice;
}

/// What changes when the boss, in its second phase, drops to half its health: it roars again and every
/// attack gains something. Like the second phase, these are numbers that replace the ones before.
struct FrenzyStats {
    float healthFraction = 0.5f; ///< Of the second phase's bar.
    AttackChoiceStats choice = FrenzyChoice();
    ComboStats combo{.chance = 0.65f, .warningScale = 0.7f, .finalRecoverySeconds = 0.9f};
    float chargeBoltIntervalSeconds = 0.15f; ///< Strands thrown while it runs.
    int ringWaves = 2;                       ///< Rings per Hair Ring.
    float ringGapTurnDegrees = 30.0f;        ///< How far the gap turns from one ring to the next.
    float leapShadowSpeed = 240.0f;          ///< Still slower than the player's 300.
    int leapLandingStrands = 12;
    int volleyBursts = 5; ///< Aimed, led, aimed, led, aimed.
    float volleyBurstIntervalSeconds = 0.25f;
    int summonCount = 6; ///< Hairballs per Summon.
    ShockwaveStats roar{.speed = 380.0f, .reach = 300.0f, .thickness = 20.0f, .damage = 20};
};

/// The boss's two phases and the transformation between them.
enum class BossPhase {
    One,          ///< The three attacks. Emptying the bar starts the transformation, not the victory.
    Transforming, ///< Invulnerable for `PhaseTwoStats::transformSeconds`, then the second phase.
    Two,          ///< Faster, with the Volley. Emptying the bar wins the fight.
};

} // namespace horde
