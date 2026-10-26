#include "app/render/BossDraw.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "app/RaylibConvert.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"

namespace horde {

namespace {

// The charge. During the warning the boss crouches (its body shrinks by this fraction) and, once the aim
// locks, trembles sideways. While it runs it leaves a few ghosts behind. Stunned against a wall, it has
// stars circling over its head.
constexpr float kChargeCrouch = 0.12f;
constexpr float kChargeTremble = 2.5f;
constexpr int kChargeGhosts = 3;
constexpr int kStunStars = 3;
constexpr float kStunStarRadius = 5.0f;
// The shield: a coloured band just outside the tips of the tufts, over a faint bubble. Its brightness
// follows the strength left; a hit that counts flashes it white, and a bounce flashes it thicker. The
// Barrier breathes, so it reads as alive and growing back.
constexpr float kShieldGap = 6.0f;
constexpr float kShieldWidth = 5.0f;
constexpr float kShieldBounceWidth = 4.0f;
constexpr float kBarrierPulse = 6.0f;
// The Hair Ring. During the warning the boss swells by this fraction and the ring is drawn on the floor
// just outside the tips of its tufts, with the gap as a pale lane running out from it. As it spits, it
// deflates for a moment.
constexpr float kRingSwell = 0.15f;
constexpr float kRingSpitShrink = 0.08f;
constexpr float kRingMarkGap = 8.0f;
constexpr float kRingMarkWidth = 10.0f;
constexpr float kRingLaneLength = 420.0f;
constexpr float kDegreesPerRadian = 180.0f / std::numbers::pi_v<float>;
// The Leap. The boss crouches by this fraction before it jumps, then flies in an arc this high above the
// floor, drawn up to this much bigger at the top, as if closer to the camera. It lands a little squashed.
// Its shadow grows from a fraction of the body to the whole landing zone.
constexpr float kLeapCrouch = 0.15f;
constexpr float kLeapHeight = 220.0f;
constexpr float kLeapGrow = 0.3f;
constexpr float kLeapLandingSquash = 0.12f;
constexpr float kLeapShadowStart = 0.3f;
// The Aimed Volley. The boss leans back through the warning (shrinks by this fraction), with a line on the
// floor toward the player, and swells for a moment with each burst.
constexpr float kVolleyLean = 0.08f;
constexpr float kVolleyBurstSwell = 0.08f;
constexpr float kVolleyBurstFlash = 0.08f;
constexpr float kVolleyLineLength = 600.0f;
// The Summon. The boss swells and shudders as it howls.
constexpr float kSummonSwell = 0.12f;
constexpr float kSummonShudder = 2.0f;
// The transformation: the body swells and trembles harder as it goes, and the eyes flicker from amber to
// red, faster toward the end.
constexpr float kTransformSwell = 0.2f;
constexpr float kTransformTremble = 4.0f;

constexpr HairStyle kBossStyle{kBossFill, kBossEdge, kBossEye, 28, 6.0f, 5.0f, 0.45f};
// In the second phase the eyes turn red, so the change reads at a glance.
constexpr HairStyle kEnragedBossStyle{kBossFill, kBossEdge, kBossEyeEnraged, 28, 6.0f, 5.0f, 0.45f};
// In a frenzy it bristles (more tufts) and reddens, over a pulsing glow on the floor.
constexpr HairStyle kFrenziedBossStyle{kFrenzyFill, kBossEdge, kBossEyeEnraged, 40, 6.0f, 5.0f, 0.45f};
constexpr float kFrenzyGlow = 0.7f;
// A dead boss turns grey like a dead player, and its eyes go dark.
constexpr HairStyle kDeadBossStyle{kPlayerDeadFill, kPlayerDeadEdge, kPlayerDeadEdge, 28, 6.0f, 5.0f, 0.0f};

bool InAttack(const Boss& boss, AttackKind kind, AttackStage stage) {
    return boss.attack && boss.attack->kind == kind && boss.attack->stage == stage;
}

// The charge's path on the floor, drawn by the same `PlanChargePath` the run follows. Faint while it still
// follows the player, bright once it locks, which is the moment to get out of the way. A bar across the
// end marks where the boss will hit the wall and be stunned.
void DrawChargeTelegraph(const Boss& boss, const Arena& arena) {
    if (IsDead(boss.health) || !InAttack(boss, AttackKind::Charge, AttackStage::Warning)) {
        return;
    }
    const BossAttack& charge = *boss.attack;
    const ChargeStats& stats = boss.charge;
    const Vec2 start = boss.motion.position;
    const ChargePath path = PlanChargePath(start, charge.direction, stats.distance, boss.radius, arena);
    // To the front of the body where the run ends, not to its centre.
    const Vec2 end = path.end + charge.direction * boss.radius;
    const Vec2 side = Vec2{-charge.direction.y, charge.direction.x} * boss.radius;
    const float warning = WarningSeconds(boss, charge);
    const bool locked = charge.stageSeconds > warning - stats.aimLockSeconds;
    const float progress = std::min(charge.stageSeconds / warning, 1.0f);

    DrawLineEx(ToRaylib(start), ToRaylib(end), boss.radius * 2.0f,
               Fade(kBlood, locked ? 0.3f : 0.06f + 0.12f * progress));
    const Color edge = Fade(kBlood, locked ? 0.95f : 0.4f);
    DrawLineEx(ToRaylib(start + side), ToRaylib(end + side), 2.0f, edge);
    DrawLineEx(ToRaylib(start - side), ToRaylib(end - side), 2.0f, edge);
    if (path.endsAtWall) {
        DrawLineEx(ToRaylib(end + side), ToRaylib(end - side), 6.0f, edge);
    }
}

// The Leap's landing on the floor: a shadow that grows as the boss comes down, and the edge of the landing
// zone around it, faint while it still follows the player and bright once it locks.
void DrawLeapTelegraph(const Boss& boss) {
    if (IsDead(boss.health) || !InAttack(boss, AttackKind::Leap, AttackStage::Warning)) {
        return;
    }
    const BossAttack& leap = *boss.attack;
    const LeapStats& stats = boss.leap;
    const float airSeconds = LeapAirSeconds(boss, leap);
    const float airborne = std::max(leap.stageSeconds - LeapCrouchSeconds(boss, leap), 0.0f);
    const float progress = std::min(airborne / airSeconds, 1.0f);
    const bool locked = airborne > airSeconds - stats.lockSeconds;
    const Vector2 center = ToRaylib(leap.target);

    const float shadow = stats.impactRadius * (kLeapShadowStart + (1.0f - kLeapShadowStart) * progress);
    DrawCircleV(center, shadow, Fade(BLACK, 0.2f + 0.35f * progress));
    const float edge = locked ? 4.0f : 2.0f;
    DrawRing(center, stats.impactRadius - edge, stats.impactRadius, 0.0f, 360.0f, 64,
             Fade(kBlood, locked ? 0.95f : 0.25f + 0.25f * progress));
}

// The Hair Ring on the floor while the boss swells: a red band around the body wherever a strand will leave,
// thicker and brighter as the spit comes closer, and the gap as a pale lane running out from the body, so
// the player can see from anywhere which way is safe.
void DrawHairRingTelegraph(const Boss& boss) {
    if (IsDead(boss.health) || !InAttack(boss, AttackKind::HairRing, AttackStage::Warning)) {
        return;
    }
    const BossAttack& ring = *boss.attack;
    const HairRingStats& stats = boss.hairRing;
    const float progress = std::min(ring.stageSeconds / WarningSeconds(boss, ring), 1.0f);
    const Vector2 center = ToRaylib(boss.motion.position);
    // In degrees, clockwise on screen, the way raylib draws arcs.
    const float gapMiddle = std::atan2(ring.direction.y, ring.direction.x) * kDegreesPerRadian;
    const float gapHalf = 180.0f * static_cast<float>(stats.gapSlots) / static_cast<float>(stats.slots);

    DrawCircleSector(center, kRingLaneLength, gapMiddle - gapHalf, gapMiddle + gapHalf, 24,
                     Fade(kRingLane, 0.06f + 0.08f * progress));
    for (const float edge : {gapMiddle - gapHalf, gapMiddle + gapHalf}) {
        const Vec2 out = Rotated(Vec2{1.0f, 0.0f}, edge / kDegreesPerRadian);
        DrawLineEx(ToRaylib(boss.motion.position + out * boss.radius),
                   ToRaylib(boss.motion.position + out * kRingLaneLength), 2.0f, Fade(kRingLane, 0.5f));
    }

    // Outside the tips of the swollen tufts, so the hair never covers it.
    const float inner = boss.radius * (1.0f + kRingSwell) * (1.0f + kTuftLength) + kRingMarkGap;

    DrawRing(center, inner, inner + kRingMarkWidth * (0.4f + 0.6f * progress), gapMiddle + gapHalf,
             gapMiddle + 360.0f - gapHalf, 64, Fade(kBlood, 0.3f + 0.6f * progress));
}

// The Volley's aim on the floor: a thin line from the body toward the player, brighter as the first burst
// comes closer. It follows the player, as the bursts do.
void DrawVolleyTelegraph(const Boss& boss) {
    if (IsDead(boss.health) || !InAttack(boss, AttackKind::Volley, AttackStage::Warning)) {
        return;
    }
    const BossAttack& volley = *boss.attack;
    const float progress = std::min(volley.stageSeconds / WarningSeconds(boss, volley), 1.0f);
    const Vec2 start = boss.motion.position + volley.direction * boss.radius;
    const Vec2 end = start + volley.direction * kVolleyLineLength;
    DrawLineEx(ToRaylib(start), ToRaylib(end), 2.0f + 2.0f * progress, Fade(kBlood, 0.2f + 0.6f * progress));
}

void DrawStunStars(Vec2 position, float radius, float seconds) {
    for (int i = 0; i < kStunStars; ++i) {
        const float angle = seconds * 5.0f + 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / kStunStars;
        const Vec2 orbit{std::cos(angle) * radius * 0.6f, std::sin(angle) * radius * 0.2f};
        const Vec2 star = position + Vec2{0.0f, -radius * 1.15f} + orbit;
        DrawCircleV(ToRaylib(star), kStunStarRadius, kStunStar);
    }
}

// Around the body as it is drawn, so it follows the crouch, the swell and the jump.
void DrawBossShield(const Shield& shield, Vec2 position, float radius, float seconds) {
    if (!IsShieldUp(shield)) {
        return;
    }
    const float left = shield.strength / MaxStrength(shield.stats, shield.kind);
    float inner = radius * (1.0f + kTuftLength) + kShieldGap;
    if (shield.kind == ShieldKind::Barrier) {
        inner += kBarrierPulse * 0.5f * (1.0f + std::sin(seconds * 6.0f));
    }
    const float width = kShieldWidth + (shield.bounceFlashSeconds > 0.0f ? kShieldBounceWidth : 0.0f);
    const Color band = shield.hitFlashSeconds > 0.0f ? kExposed : ShieldColor(shield.kind);
    DrawCircleV(ToRaylib(position), inner, Fade(ShieldColor(shield.kind), 0.08f + 0.08f * left));
    DrawRing(ToRaylib(position), inner, inner + width, 0.0f, 360.0f, 64, Fade(band, 0.35f + 0.6f * left));
}

// Every pose of the boss ends here: the body, its shield around it, and stars while its broken shield
// staggers it.
void DrawBossBody(const Boss& boss, Vec2 position, float radius, Vec2 lookAt) {
    const HairStyle* style = &kBossStyle;
    if (boss.frenzied) {
        style = &kFrenziedBossStyle;
        const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 8.0f);
        DrawCircleV(ToRaylib(position), radius * (1.0f + kFrenzyGlow), Fade(kBlood, 0.08f + 0.1f * pulse));
    } else if (boss.phase == BossPhase::Two) {
        style = &kEnragedBossStyle;
    }
    DrawHairBody(position, radius, lookAt, *style, boss.hitFlashSeconds > 0.0f);
    DrawBossShield(boss.shield, position, radius, static_cast<float>(GetTime()));
    if (IsStaggered(boss)) {
        DrawStunStars(position, radius, boss.staggerSeconds);
    }
}

// Crouched and trembling through the warning, with ghosts behind it while it runs and stars over its head
// while it is stunned. The eyes look down its path instead of at the player.
void DrawChargingBoss(const Boss& boss, Vec2 position) {
    const BossAttack& attack = *boss.attack;
    const ChargeStats& stats = boss.charge;
    float radius = boss.radius;
    switch (attack.stage) {
    case AttackStage::Warning: {
        const float warning = WarningSeconds(boss, attack);
        radius *= 1.0f - kChargeCrouch * std::min(attack.stageSeconds / warning, 1.0f);
        if (attack.stageSeconds > warning - stats.aimLockSeconds) {
            const Vec2 side{-attack.direction.y, attack.direction.x};
            position += side * (kChargeTremble * std::sin(attack.stageSeconds * 90.0f));
        }
        break;
    }
    case AttackStage::Active:
        for (int i = kChargeGhosts; i >= 1; --i) {
            const Vec2 ghost = position - attack.direction * (radius * 0.5f * static_cast<float>(i));
            DrawCircleV(ToRaylib(ghost), radius * (1.0f - 0.1f * static_cast<float>(i)),
                        Fade(kBossFill, 0.35f / static_cast<float>(i)));
        }
        break;
    case AttackStage::Recovery:
        break;
    }
    DrawBossBody(boss, position, radius, position + attack.direction);
    if (attack.stage == AttackStage::Recovery && attack.stunned) {
        DrawStunStars(position, radius, attack.stageSeconds);
    }
}

// Swells through the warning, deflates as it spits, and is back to its size while it recovers. It watches
// the player the whole time.
void DrawSpittingBoss(const Boss& boss, Vec2 position, Vec2 playerPosition) {
    const BossAttack& ring = *boss.attack;
    float radius = boss.radius;
    if (ring.stage == AttackStage::Warning) {
        radius *= 1.0f + kRingSwell * std::min(ring.stageSeconds / WarningSeconds(boss, ring), 1.0f);
    } else if (ring.stage == AttackStage::Active) {
        radius *= 1.0f - kRingSpitShrink;
    }
    DrawBossBody(boss, position, radius, playerPosition);
}

// Crouches, flies up in an arc toward its shadow, growing as it rises, and lands squashed. In the air it is
// drawn above everything else, since it is over the player's head.
void DrawLeapingBoss(const Boss& boss, Vec2 position, Vec2 playerPosition) {
    const BossAttack& leap = *boss.attack;
    float radius = boss.radius;
    switch (leap.stage) {
    case AttackStage::Warning: {
        const float crouch = LeapCrouchSeconds(boss, leap);
        const float airborne = leap.stageSeconds - crouch;
        if (airborne <= 0.0f) {
            radius *= 1.0f - kLeapCrouch * leap.stageSeconds / crouch;
            break;
        }
        const float air = std::min(airborne / LeapAirSeconds(boss, leap), 1.0f);
        const float arc = std::sin(std::numbers::pi_v<float> * air);
        position.y -= kLeapHeight * arc;
        radius *= 1.0f + kLeapGrow * arc;
        break;
    }
    case AttackStage::Active:
        radius *= 1.0f + kLeapLandingSquash;
        break;
    case AttackStage::Recovery:
        break;
    }
    DrawBossBody(boss, position, radius, playerPosition);
}

// Leans back through the warning, and swells for a moment with every burst. It watches the player.
void DrawVolleyingBoss(const Boss& boss, Vec2 position, Vec2 playerPosition) {
    const BossAttack& volley = *boss.attack;
    float radius = boss.radius;
    if (volley.stage == AttackStage::Warning) {
        radius *= 1.0f - kVolleyLean * std::min(volley.stageSeconds / WarningSeconds(boss, volley), 1.0f);
    } else if (volley.stage == AttackStage::Active) {
        const float sinceBurst = std::fmod(volley.stageSeconds, boss.volley.burstIntervalSeconds);
        if (sinceBurst < kVolleyBurstFlash) {
            radius *= 1.0f + kVolleyBurstSwell;
        }
    }
    DrawBossBody(boss, position, radius, playerPosition);
}

// Swells and shudders while it howls, and settles back as the hairballs come out.
void DrawSummoningBoss(const Boss& boss, Vec2 position, Vec2 playerPosition) {
    const BossAttack& summon = *boss.attack;
    float radius = boss.radius;
    if (summon.stage == AttackStage::Warning) {
        radius *= 1.0f + kSummonSwell * std::min(summon.stageSeconds / WarningSeconds(boss, summon), 1.0f);
        position.x += kSummonShudder * std::sin(summon.stageSeconds * 60.0f);
    }
    DrawBossBody(boss, position, radius, playerPosition);
}

// Swells and trembles harder as the transformation goes on, with its eyes flickering to red, faster toward
// the end. No shield is drawn: nothing can hurt it.
void DrawTransformingBoss(const Boss& boss, Vec2 position, Vec2 playerPosition) {
    const float total = boss.phaseTwo.transformSeconds;
    const float progress = std::clamp(1.0f - boss.transformSecondsLeft / total, 0.0f, 1.0f);
    const float seconds = total - boss.transformSecondsLeft;
    position.x += kTransformTremble * progress * std::sin(seconds * 70.0f);
    const float radius = boss.radius * (1.0f + kTransformSwell * progress);
    const bool red = std::sin(seconds * (6.0f + 30.0f * progress)) > 0.0f;
    DrawHairBody(position, radius, playerPosition, red ? kEnragedBossStyle : kBossStyle, false);
}

} // namespace

void DrawBossTelegraphs(const Boss& boss, const Arena& arena) {
    DrawChargeTelegraph(boss, arena);
    DrawHairRingTelegraph(boss);
    DrawLeapTelegraph(boss);
    DrawVolleyTelegraph(boss);
}

Color ShieldColor(ShieldKind kind) {
    return kind == ShieldKind::Shell ? kShell : kBarrier;
}

void DrawBoss(const Boss& boss, Vec2 playerPosition, float alpha) {
    const Vec2 position = Lerp(boss.motion.previousPosition, boss.motion.position, alpha);
    if (IsDead(boss.health)) {
        DrawHairBody(position, boss.radius, playerPosition, kDeadBossStyle, false);
        return;
    }
    if (IsTransforming(boss)) {
        DrawTransformingBoss(boss, position, playerPosition);
        return;
    }
    if (!boss.attack) {
        DrawBossBody(boss, position, boss.radius, playerPosition);
        return;
    }
    switch (boss.attack->kind) {
    case AttackKind::Charge:
        DrawChargingBoss(boss, position);
        break;
    case AttackKind::HairRing:
        DrawSpittingBoss(boss, position, playerPosition);
        break;
    case AttackKind::Leap:
        DrawLeapingBoss(boss, position, playerPosition);
        break;
    case AttackKind::Volley:
        DrawVolleyingBoss(boss, position, playerPosition);
        break;
    case AttackKind::Summon:
        DrawSummoningBoss(boss, position, playerPosition);
        break;
    }
}

} // namespace horde
