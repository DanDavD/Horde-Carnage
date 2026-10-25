#pragma once

#include <array>
#include <optional>

#include "core/Random.h"

namespace horde {

/// The boss's three attacks of the first phase.
enum class AttackKind {
    Charge,   ///< Crouches, then runs in a straight line across the arena.
    HairRing, ///< Swells, then spits a ring of hair in every direction but one gap.
    Leap,     ///< Jumps, comes down on its shadow and sends a shockwave across the floor.
};

/// How likely one attack is to be picked, at three distances from the player. Between two of them the
/// weight blends in a straight line; closer than the near distance or farther than the far one it holds.
/// Only the ratios between the attacks matter, not the numbers themselves.
struct AttackWeights {
    float nearWeight = 1.0f;
    float midWeight = 1.0f;
    float farWeight = 1.0f;
};

/// How the boss picks its next attack, and how long it stalks before it.
///
/// Each attack is best at its own distance: the Charge crosses the arena, the Leap closes a medium gap, and
/// the Hair Ring is hardest to dodge up close, where the strands are tight. So no distance is safe to stand
/// at: running far invites the Charge, and hugging the boss invites the Ring.
struct AttackChoiceStats {
    float nearDistance = 250.0f; ///< Between the centres of the boss and the player.
    float farDistance = 550.0f;  ///< The mid weights apply halfway between the two.
    AttackWeights charge{.nearWeight = 1.0f, .midWeight = 2.0f, .farWeight = 4.0f};
    AttackWeights hairRing{.nearWeight = 4.0f, .midWeight = 2.0f, .farWeight = 1.0f};
    AttackWeights leap{.nearWeight = 1.0f, .midWeight = 4.0f, .farWeight = 2.0f};
    /// Stalking time between attacks, picked anew each time, so the next attack cannot be counted down to.
    float minStalkSeconds = 0.8f;
    float maxStalkSeconds = 1.6f;
    /// Every warning is stretched or shortened by a factor in this range, so the player has to watch the
    /// boss instead of dodging to a rhythm. The moments that decide a dodge (the Charge's aim lock and the
    /// Leap's shadow lock) keep their exact length.
    float minWarningScale = 0.85f;
    float maxWarningScale = 1.15f;
};

/// How the boss chains two attacks into a combo.
struct ComboStats {
    float chance = 0.35f; ///< That an attack which can open a combo does.
    /// The second attack's warning is this much shorter: the player is still busy with the first one, and
    /// has less time to read the second.
    float warningScale = 0.7f;
    /// The recovery after the second attack lasts at least this long, the reward for getting through the
    /// whole chain. The first attack has no recovery at all: the second one starts in its place.
    float finalRecoverySeconds = 1.2f;
};

/// Two attacks the boss can chain without stalking in between.
struct Combo {
    AttackKind opener;
    AttackKind followUp;
};

/// Every combo the boss knows. The Rebound runs back the way it came, so dodging the first charge is not
/// the end of it. The Pursuit leaps after a player who ran far from the charge. The Crash spits a ring on
/// landing, so the shockwave and the gap have to be read one after the other. A Hair Ring opens none.
inline constexpr std::array<Combo, 3> kCombos{{
    {.opener = AttackKind::Charge, .followUp = AttackKind::Charge},
    {.opener = AttackKind::Charge, .followUp = AttackKind::Leap},
    {.opener = AttackKind::Leap, .followUp = AttackKind::HairRing},
}};

/// How likely `kind` is to be picked with the player `distance` units away.
[[nodiscard]] float AttackWeightAt(const AttackChoiceStats& stats, AttackKind kind, float distance);

/// Picks the next attack at random, weighted by `AttackWeightAt`. Never `last`, the attack that just ended,
/// so the same attack never comes twice in a row outside a combo.
AttackKind ChooseAttack(const AttackChoiceStats& stats, float distance, std::optional<AttackKind> last,
                        Random& random);

/// Whether an attack of kind `opener` opens a combo this time, and which attack follows it if it does.
std::optional<AttackKind> ChooseFollowUp(const ComboStats& stats, AttackKind opener, Random& random);

} // namespace horde
