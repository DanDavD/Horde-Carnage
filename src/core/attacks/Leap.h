#pragma once

#include "core/Arena.h"
#include "core/BossAttack.h"
#include "core/Shockwave.h"
#include "core/Vec2.h"

namespace horde {

/// The numbers of the Leap.
struct LeapStats {
    float crouchSeconds = 0.35f; ///< On the ground, gathering itself, before it leaves it.
    /// In the air, out of reach, while its shadow on the floor shows where it will land.
    float airSeconds = 1.0f;
    /// The last part of the air time, in which the shadow stops following the player. Running out from
    /// under it before then only drags it along.
    float lockSeconds = 0.35f;
    /// How fast the shadow follows the player, in units per second. A third slower than the player runs, so
    /// a player who keeps moving from the takeoff gets out from under it. One who only starts running when
    /// it locks covers about 100 units, not enough to leave the landing zone: then only the dash helps.
    float shadowSpeed = 200.0f;
    /// The landing zone around the shadow's centre, a little wider than the body. A player inside it when
    /// the boss lands takes the impact.
    float impactRadius = 100.0f;
    int impactDamage = 35;
    float impactKnockback = 600.0f; ///< Units per second added to the player, away from the landing.
    float landingSeconds = 0.15f;   ///< The impact itself, the active stage.
    float recoverySeconds = 1.0f;   ///< Sunk into the floor after landing: the opening to punish it.
    ShockwaveStats shockwave;       ///< The ring that leaves the landing point.
    /// In a frenzy only: a ring of strands with no gap leaves the landing too, one of them at the player.
    /// 0 spits none.
    int landingStrands = 0;
};

/// Points a new Leap: it takes off from `takeoff`, and its shadow starts under the player, kept far enough
/// inside the walls for a body of `radius` to land there.
void AimLeap(BossAttack& leap, Vec2 takeoff, Vec2 playerPosition, float radius, const Arena& arena);

/// The part of a Leap's warning spent crouched on the ground, scaled like the rest of the warning.
[[nodiscard]] float LeapCrouchSeconds(const LeapStats& stats, const BossAttack& leap);

/// The part of a Leap's warning spent in the air, ending with the shadow's lock, which keeps its full length.
[[nodiscard]] float LeapAirSeconds(const LeapStats& stats, const BossAttack& leap);

/// Whether `leap` is in the air. Up there the attacker is not a body: it does not collide, take bullets or
/// hurt on touch.
[[nodiscard]] bool IsLeapAirborne(const LeapStats& stats, const BossAttack& leap);

/// What one step of a Leap asks of the boss that runs it.
struct LeapStep {
    bool landed = false;   ///< It just came down: the impact, and a shockwave from its body.
    bool finished = false; ///< The recovery is over: the attacker goes back to stalking.
};

/// Advances a Leap by one step. Crouched, the attacker holds its ground; in the air its body travels from the
/// takeoff to the shadow, which chases the player until it locks; it lands on the shadow, then recovers. It
/// moves `attacker.motion`.
LeapStep StepLeap(BossAttack& leap, const Attacker& attacker, const LeapStats& stats, Vec2 playerPosition,
                  const Arena& arena, float dt);

} // namespace horde
