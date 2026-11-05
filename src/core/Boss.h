#pragma once

#include <optional>
#include <string_view>

#include "core/Arena.h"
#include "core/AttackChoice.h"
#include "core/AttackClock.h"
#include "core/BossAttack.h"
#include "core/BossId.h"
#include "core/Health.h"
#include "core/Movement.h"
#include "core/Projectile.h"
#include "core/Random.h"
#include "core/Shield.h"
#include "core/Shockwave.h"
#include "core/SpawnMarker.h"
#include "core/Vec2.h"
#include "core/attacks/Charge.h"
#include "core/attacks/HairRing.h"
#include "core/attacks/Leap.h"
#include "core/attacks/Summon.h"
#include "core/attacks/Volley.h"
#include "core/bosses/MatriarchStats.h"

namespace horde {

/// What a boss step asks of the world. The boss decides when to spit and when it lands, but its
/// projectiles and shockwaves live in the world's pools, and the impact hurts the player, so the world
/// does those.
struct BossActions {
    bool spitHairRing = false; ///< Spawn a ring now, around the gap in `BossAttack::direction`.
    bool landed = false;       ///< It just came down from a Leap: the impact, and a shockwave from its body.
    bool spitVolley = false;   ///< Spawn one burst of a Volley now, aimed along `BossAttack::direction`.
    bool throwBolts = false;   ///< In a frenzy, while it charges: a strand at the player and one ahead.
    bool summon = false;       ///< Put the Summon's marks on the floor around the player.
    bool roared = false;       ///< The transformation is over: a shockwave from its body.
    bool hitWall = false;      ///< A Charge just ended against a wall, and the boss is stunned.
    bool attackStarted = false; ///< An attack began its warning in this step, `BossAttack::kind` says which.
};

/// The Matriarch, the one boss the game is about: a giant hairball, mother of the small ones.
///
/// It has the same body components as the player and the hairballs (`Motion`, `MovementStats`, a radius
/// and a weight), so it moves, collides and stays inside the walls through the same systems. It is a type
/// of its own rather than an `Enemy` with bigger numbers because its rules differ: bullets neither stagger
/// nor push it, and its attacks run on their own state machine.
///
/// Between attacks it stalks: it walks at the player for a random stretch of time. Then it attacks, and the
/// attack runs through its warning, active and recovery stages before it stalks again. Which attack comes
/// next is picked at random, weighted by the distance to the player, never the same one twice in a row
/// (see `ChooseAttack`). Sometimes an attack opens a combo, and a second one follows it at once (see
/// `kCombos`).
///
/// When its health first runs out it does not die: it transforms, and the second phase starts with its
/// health full (`BossPhase`). Only the second phase's bar wins the fight.
///
/// A dead boss takes no part in the simulation: it does not chase, attack, block, take bullets or hurt.
struct Boss {
    BossId id = BossId::Matriarch; ///< Which boss this is: picks its row in the table of `BossRules`.
    Motion motion;
    /// Slower than a hairball (160) when it walks, and slow to get going. It is not the walk that makes it
    /// dangerous, it is the attacks.
    MovementStats stats{.maxSpeed = 120.0f, .acceleration = 400.0f, .deceleration = 1500.0f};
    float radius = 60.0f; ///< Three hairballs across.
    /// Ten times the player: the player cannot shove it, and it pushes the player out of its way.
    float weight = 40.0f;
    /// Twice a hairball's touch. Five touches kill a player at full health.
    int contactDamage = 20;
    /// 120 pistol bullets or 200 SMG bullets, but only while its shield is down. No invulnerability window,
    /// like the hairballs, so every bullet counts.
    Health health{.current = 1200, .max = 1200, .invulnerabilityAfterHit = 0.0f};
    float hitFlashSeconds = 0.0f; ///< Time left on the white flash of the last hit.
    /// Stands between the bullets and the health. It has to be broken, with the right gun, before the boss
    /// can be hurt at all.
    Shield shield;
    /// Breaking the shield staggers the boss: it drops its attack and stands still for this long.
    float breakStaggerSeconds = 1.0f;
    float staggerSeconds = 0.0f; ///< Time left in the current stagger.

    std::optional<BossAttack> attack; ///< Empty while it stalks the player between attacks.
    /// Stalking time left before the next attack starts. The first stalk is longer than the ones that
    /// follow, so the fight opens with a moment to look at her.
    float secondsUntilAttack = 2.0f;
    /// The attack after this stalk, when it is already decided; otherwise it is picked when the stalk ends.
    /// The fight opens with a Charge, the plainest attack to read.
    std::optional<AttackKind> nextAttack = AttackKind::Charge;
    std::optional<AttackKind> lastAttack; ///< The attack that ended last, which the next pick skips.
    int ringsStarted = 0; ///< Hair Rings started this fight. The gap opens on one side, then the other.
    ChargeStats charge;
    HairRingStats hairRing;
    LeapStats leap;
    VolleyStats volley;
    SummonStats summon;
    AttackChoiceStats choice; ///< The phase in progress: the second phase replaces it.
    ComboStats combo;         ///< The same.
    BossPhase phase = BossPhase::One;
    float transformSecondsLeft = 0.0f; ///< Above 0 while it transforms.
    PhaseTwoStats phaseTwo;
    bool frenzied = false; ///< In the second phase, below `FrenzyStats::healthFraction` of its health.
    FrenzyStats frenzy;
};

/// Advances the boss by one fixed step. The hit flash fades and a dead boss only brakes to a stop, whoever
/// the boss is; everything else (the walk while it stalks, its attacks, its phases) is the step function in
/// its `BossRules`. Pushing bodies apart, bullets and damage to the player are handled by `StepWorld`. Every
/// random pick draws from `random`.
///
/// @param player Where the player is and how fast they move: the Volley leads a moving player.
/// @return What the world has to do for the boss in this step, such as spawning a ring of hair.
BossActions StepBoss(Boss& boss, const Motion& player, float playerRadius, const Arena& arena, Random& random,
                     float dt);

/// The step of one living boss, past what `StepBoss` does for every boss.
using BossStepFunction = BossActions (*)(Boss& boss, const Motion& player, float playerRadius, const Arena& arena,
                                         Random& random, float dt);

/// One row of the table of bosses: its name and the plain functions that make and run it. A table of function
/// pointers instead of a class with virtual functions, so `Boss` and `World` stay plain data that can be
/// copied, compared and stepped the same way every time.
struct BossRules {
    BossId id;
    std::string_view name;       ///< As the game shows it, in title case.
    Boss (*make)(Vec2 position); ///< A fresh boss of this kind, standing still at `position`.
    BossStepFunction step;
};

/// The row of the table for `id`. Adding a boss is a new `BossId`, its file in `core/bosses/` and one row.
[[nodiscard]] const BossRules& RulesOf(BossId id);

/// A fresh boss of kind `id`, standing still at `position`.
[[nodiscard]] Boss MakeBoss(BossId id, Vec2 position);

/// How long the warning of `attack` lasts: its stats' warning, scaled by the attack's `warningScale`. For a
/// Leap, the crouch and the air time together. Never shorter than the aim or shadow lock at its end.
[[nodiscard]] float WarningSeconds(const Boss& boss, const BossAttack& attack);

/// The part of a Leap's warning spent crouched on the ground, scaled like the rest of the warning.
[[nodiscard]] float LeapCrouchSeconds(const Boss& boss, const BossAttack& leap);

/// The part of a Leap's warning spent in the air, ending with the shadow's lock.
[[nodiscard]] float LeapAirSeconds(const Boss& boss, const BossAttack& leap);

/// Whether the boss is in the middle of a run. A running boss goes through the player instead of shoving
/// them, and hits harder.
[[nodiscard]] bool IsCharging(const Boss& boss);

/// Whether the boss is in the air during a Leap. Up there it is not a body: it does not collide, take
/// bullets or hurt on touch.
[[nodiscard]] bool IsAirborne(const Boss& boss);

/// Where a strand leaving the boss now meets a player who keeps their velocity, at most
/// `VolleyStats::maxLeadSeconds` ahead and inside the arena.
[[nodiscard]] Vec2 LeadPoint(const Boss& boss, const Motion& player, const Arena& arena);

/// One strand of a Volley's burst, leaving the edge of the body in `direction` (length 1).
[[nodiscard]] Projectile MakeVolleyStrand(const Boss& boss, Vec2 direction);

/// One strand of the boss's ring, leaving the edge of its body in `direction` (length 1), with the ring's
/// speed, range, size and damage.
[[nodiscard]] Projectile MakeHairStrand(const Boss& boss, Vec2 direction);

} // namespace horde
