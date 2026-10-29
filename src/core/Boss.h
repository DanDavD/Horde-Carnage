#pragma once

#include <optional>

#include "core/Arena.h"
#include "core/AttackChoice.h"
#include "core/Health.h"
#include "core/Movement.h"
#include "core/Projectile.h"
#include "core/Random.h"
#include "core/Shield.h"
#include "core/Shockwave.h"
#include "core/SpawnMarker.h"
#include "core/Vec2.h"

namespace horde {

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

/// Advances the boss by one fixed step: the walk while it stalks, and every stage of its attack. Pushing
/// bodies apart, bullets and damage to the player are handled by `StepWorld`. Every random pick (the next
/// attack, the stalk, the warning's length and the combos) draws from `random`.
///
/// @param player Where the player is and how fast they move: the Volley leads a moving player.
/// @return What the world has to do for the boss in this step, such as spawning a ring of hair.
BossActions StepBoss(Boss& boss, const Motion& player, float playerRadius, const Arena& arena, Random& random,
                     float dt);

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

/// Whether the boss is between its two phases: invulnerable, still and roaring.
[[nodiscard]] bool IsTransforming(const Boss& boss);

/// Starts the transformation into the second phase: the attack is dropped, the health is refilled at once
/// (the bar draws it filling up), and nothing hurts the boss until it ends. Only from the first phase.
void StartTransformation(Boss& boss);

/// Whether the boss has just dropped low enough in its second phase to fall into a frenzy.
[[nodiscard]] bool ShouldFrenzy(const Boss& boss);

/// Puts the boss in a frenzy: `FrenzyStats` replace the numbers of the second phase.
void EnterFrenzy(Boss& boss);

/// How far into the Summon's far range the player stands, from 0 inside `midDistance` (400) to 1 at or
/// past `farDistance` (550), eased with smoothstep. Zero in the first phase, while transforming, or once
/// the boss is dead.
[[nodiscard]] float SummonThreat(const Boss& boss, float distance);

/// Where a strand leaving the boss now meets a player who keeps their velocity, at most
/// `VolleyStats::maxLeadSeconds` ahead and inside the arena.
[[nodiscard]] Vec2 LeadPoint(const Boss& boss, const Motion& player, const Arena& arena);

/// One strand of a Volley's burst, leaving the edge of the body in `direction` (length 1).
[[nodiscard]] Projectile MakeVolleyStrand(const Boss& boss, Vec2 direction);

/// Whether the boss is reeling from its broken shield.
[[nodiscard]] bool IsStaggered(const Boss& boss);

/// Cuts the boss's attack off and holds it still for `breakStaggerSeconds`, with the shortest stalk after
/// it. Called when its shield breaks, so the break is felt, not only seen on a bar.
void StaggerBoss(Boss& boss);

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
