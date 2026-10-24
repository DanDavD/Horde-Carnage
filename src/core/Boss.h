#pragma once

#include <optional>

#include "core/Arena.h"
#include "core/Health.h"
#include "core/Movement.h"
#include "core/Projectile.h"
#include "core/Vec2.h"

namespace horde {

/// The boss's attacks. The Leap joins them on day 7.
enum class AttackKind {
    Charge,   ///< Crouches, then runs in a straight line across the arena.
    HairRing, ///< Swells, then spits a ring of hair in every direction but one gap.
};

/// Every attack runs through the same three stages, so the player learns one rhythm: a warning that shows
/// what is coming and where, the attack itself, and a recovery that is the opening to punish it.
enum class AttackStage {
    Warning,
    Active,
    Recovery,
};

/// The attack in progress.
struct BossAttack {
    AttackKind kind = AttackKind::Charge;
    AttackStage stage = AttackStage::Warning;
    float stageSeconds = 0.0f; ///< Time spent in the current stage.
    /// Where the attack points, length 1. Charge: where it runs; it follows the player through the warning
    /// until the aim locks. Hair Ring: the middle of the gap, fixed from the start of the warning.
    Vec2 direction{0.0f, 1.0f};
    float distanceLeft = 0.0f; ///< Charge: how much of the run is left while it is active.
    bool stunned = false;      ///< Charge: it ran into a wall, so the recovery is the long one.
    int wavesSpat = 0;         ///< Hair Ring: how many of its rings it has spat so far.
};

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
};

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

/// What a boss step asks of the world. The boss decides when to spit, but its projectiles live in the
/// world's pool, so the world spawns them.
struct BossActions {
    bool spitHairRing = false; ///< Spawn a ring now, around the gap in `BossAttack::direction`.
};

/// Where a charge would end: after its full distance, or earlier, where the body meets a wall.
struct ChargePath {
    Vec2 end;
    bool endsAtWall = false;
};

/// The Matriarch, the one boss the game is about: a giant hairball, mother of the small ones.
///
/// It has the same body components as the player and the hairballs (`Motion`, `MovementStats`, a radius
/// and a weight), so it moves, collides and stays inside the walls through the same systems. It is a type
/// of its own rather than an `Enemy` with bigger numbers because its rules differ: bullets neither stagger
/// nor push it, and its attacks run on their own state machine.
///
/// Between attacks it stalks: it walks at the player for `attackInterval` seconds. Then it attacks, and
/// the attack runs through its warning, active and recovery stages before it stalks again. For now the
/// attacks take turns: a Charge, then a Hair Ring.
///
/// A dead boss takes no part in the simulation: it does not chase, attack, block, take bullets or hurt.
struct Boss {
    Motion motion;
    /// Slower than a hairball (160) when it walks, and slow to get going. It is not the walk that makes it
    /// dangerous, it is the attacks.
    MovementStats stats{.maxSpeed = 120.0f, .acceleration = 400.0f, .deceleration = 1500.0f};
    float radius = 60.0f; ///< Three hairballs across.
    /// Ten times the player: the player cannot shove it, and it pushes the player out of its way.
    float weight = 40.0f;
    /// Twice a hairball's touch. Five touches kill a player at full health.
    int contactDamage = 20;
    /// 120 pistol bullets or 200 SMG bullets. No invulnerability window, like the hairballs, so every
    /// bullet counts.
    Health health{.current = 1200, .max = 1200, .invulnerabilityAfterHit = 0.0f};
    float hitFlashSeconds = 0.0f; ///< Time left on the white flash of the last hit.

    std::optional<BossAttack> attack; ///< Empty while it stalks the player between attacks.
    float attackInterval = 2.0f;      ///< Seconds of stalking between the end of one attack and the next.
    float secondsUntilAttack = 2.0f;  ///< Stalking time left before the next attack starts.
    AttackKind nextAttack = AttackKind::Charge;
    int ringsStarted = 0; ///< Hair Rings started this fight. The gap opens on one side, then the other.
    ChargeStats charge;
    HairRingStats hairRing;
};

/// Advances the boss by one fixed step: the walk while it stalks, and every stage of its attack. Pushing
/// bodies apart, bullets and damage to the player are handled by `StepWorld`.
///
/// @return What the world has to do for the boss in this step, such as spawning a ring of hair.
BossActions StepBoss(Boss& boss, Vec2 playerPosition, float playerRadius, const Arena& arena, float dt);

/// Whether the boss is in the middle of a run. A running boss goes through the player instead of shoving
/// them, and hits harder.
[[nodiscard]] bool IsCharging(const Boss& boss);

/// The path of a charge from `start` along `direction` (length 1), for a body of `radius`: the full
/// `distance`, or less if a wall comes first. The simulation runs along it and the warning draws it, so
/// what the player sees is what will happen.
[[nodiscard]] ChargePath PlanChargePath(Vec2 start, Vec2 direction, float distance, float radius, const Arena& arena);

/// How many strands one ring has: every slot around the body except the gap.
[[nodiscard]] int HairRingStrands(const HairRingStats& stats);

/// The direction of strand `index` (0 to `HairRingStrands` - 1) of a ring whose gap is centred on `gap`
/// (length 1). The strands go round from one edge of the gap to the other, `slots` apart.
[[nodiscard]] Vec2 HairStrandDirection(Vec2 gap, int index, const HairRingStats& stats);

/// One strand of the boss's ring, leaving the edge of its body in `direction` (length 1), with the ring's
/// speed, range, size and damage.
[[nodiscard]] Projectile MakeHairStrand(const Boss& boss, Vec2 direction);

} // namespace horde
