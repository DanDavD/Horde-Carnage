#pragma once

#include "core/BossAttack.h"
#include "core/Projectile.h"
#include "core/Vec2.h"

namespace horde {

/// The numbers of the Hair Ring. One ring by default; more waves and a gap that turns between them are
/// there for the combos and the second phase.
struct HairRingStats {
    float warningSeconds = 0.6f; ///< Swelling, with the ring and its gap drawn on the floor.
    /// The spit. The first ring leaves at its start; with more waves, the stage lasts until the last one has
    /// left, plus this.
    float activeSeconds = 0.15f;
    float recoverySeconds = 0.8f; ///< Deflated and still: the opening to punish it.
    /// Directions evenly spaced around the body, 7.5 degrees apart. Close to the boss the strands are too
    /// tight to slip between; far away they spread out.
    int slots = 48;
    int gapSlots = 6; ///< Directions left empty, centred on the gap: 45 degrees.
    /// How far around from the player the gap opens, on alternate sides from one ring to the next. Never
    /// toward the player, or standing still would be the answer.
    float gapAngleDegrees = 90.0f;
    float speed = 220.0f; ///< Units per second. Slower than the player, so the gap can be reached on foot.
    float range = 900.0f; ///< Units a strand flies at most, unless a wall stops it first.
    float strandRadius = 8.0f;
    int damage = 15; ///< Per strand. The invulnerability window after a hit lets only one land.
    int waves = 1;   ///< Rings per attack.
    float waveIntervalSeconds = 0.4f; ///< Between two rings of the same attack.
    float gapTurnDegrees = 0.0f;      ///< How far the gap turns from one ring to the next.
};

/// Points a new Hair Ring's gap `gapAngleDegrees` around from `toPlayer` (length 1): to one side, or to the
/// other when `otherSide` is set. The boss alternates the two, so reaching the gap always means moving.
void AimHairRing(BossAttack& ring, Vec2 toPlayer, bool otherSide, const HairRingStats& stats);

/// How long the warning of `ring` lasts: the stats' warning scaled by the attack's `warningScale`.
[[nodiscard]] float HairRingWarningSeconds(const HairRingStats& stats, const BossAttack& ring);

/// How many strands one ring has: every slot around the body except the gap.
[[nodiscard]] int HairRingStrands(const HairRingStats& stats);

/// The direction of strand `index` (0 to `HairRingStrands` - 1) of a ring whose gap is centred on `gap`
/// (length 1). The strands go round from one edge of the gap to the other, `slots` apart.
[[nodiscard]] Vec2 HairStrandDirection(Vec2 gap, int index, const HairRingStats& stats);

/// One strand of a ring spat by a body of `radius` at `center`, leaving its edge in `direction` (length 1),
/// with the ring's speed, range, size and damage.
[[nodiscard]] Projectile MakeHairStrand(const HairRingStats& stats, Vec2 center, float radius, Vec2 direction);

/// What one step of a Hair Ring asks of the boss that runs it.
struct HairRingStep {
    bool spit = false;     ///< Spawn a ring now, around the gap in `BossAttack::direction`.
    bool finished = false; ///< The recovery is over: the attacker goes back to stalking.
};

/// Advances a Hair Ring by one step. The attacker holds its ground through all of it: it swells, spits its
/// rings on their beats (the gap turning between them), and gets its breath back.
HairRingStep StepHairRing(BossAttack& ring, const Attacker& attacker, const HairRingStats& stats, float dt);

} // namespace horde
