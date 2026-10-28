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

// Squash and stretch. Crouching for a Charge or a Leap squashes the body along the way it is about to go;
// running stretches it along the run; the air stretches it upward and the landing flattens it; every
// Volley burst jolts it toward the player.
constexpr float kCrouchSquash = 0.88f;
constexpr float kRunStretch = 1.22f;
constexpr float kAirStretch = 1.1f;
constexpr float kLandingSquash = 0.84f;
constexpr float kBurstStretch = 1.12f;
// How far the mouth is open, from 0 (shut) to 1, when the boss has one. It grows a mouth in the second
// phase: shut in a snarl, a little open in the frenzy, and wide open whenever it roars, howls or spits.
constexpr float kFrenzyMouth = 0.25f;
constexpr float kChargeMouth = 0.45f;
constexpr float kSpitMouth = 0.85f;
constexpr float kBurstMouth = 0.6f;
// The boss's eyes light the floor around it, faintly.
constexpr float kEyeLightReach = 2.2f;
constexpr float kEyeLight = 0.07f;

// A dark, matted mass with amber eyes. In the second phase the eyes turn red and the hair bristles; in a
// frenzy it bristles all over and reddens, over a pulsing glow on the floor.
constexpr HairStyle kBossStyle{kBossFill, kBossEdge, kBossSheen, kBossEye, 24, 0.26f, 0.0f, 0.45f, 1.0f};
constexpr HairStyle kEnragedBossStyle{kBossEnragedFill, kBossEdge, kBossSheen, kBossEyeEnraged, 32, 0.36f, 0.55f,
                                      0.45f, 1.0f};
constexpr HairStyle kFrenziedBossStyle{kFrenzyFill, kBossEdge, kBossSheen, kBossEyeEnraged, 42, 0.46f, 0.95f,
                                       0.45f, 1.0f};
constexpr float kFrenzyGlow = 0.7f;
// A dead boss turns grey like a dead player, its hair falls flat and its eyes shut.
constexpr HairStyle kDeadBossStyle{kPlayerDeadFill, kPlayerDeadEdge, kPlayerDeadFill, kPlayerDeadEdge, 24, 0.26f,
                                   0.0f, 0.0f, 1.0f};
// The death: it shakes and swells for the first part, screaming, while the colour drains out of it; then it
// sinks, a little wider than long, and its eyes shut.
constexpr float kDeathConvulsionSeconds = 0.6f;
constexpr float kDeathShake = 5.0f;
constexpr float kDeathSwell = 0.12f;
constexpr float kDeathGreyStart = 0.2f; // As a share of the whole death.
constexpr float kDeathSag = 0.86f;
constexpr float kDeathEyesShut = 0.75f;

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

// The mouth the boss has when it is not roaring: none in the first phase, a shut snarl in the second, and
// a little open in the frenzy.
float RestingMouth(const Boss& boss) {
    if (boss.phase != BossPhase::Two) {
        return -1.0f;
    }
    return boss.frenzied ? kFrenzyMouth : 0.0f;
}

// A wider mouth for a moment of the attack, only once the boss has a mouth at all.
void OpenMouth(const Boss& boss, HairPose& pose, float open) {
    if (boss.phase == BossPhase::Two) {
        pose.mouth = std::max(pose.mouth, open);
    }
}

// The style of the phase the boss is in.
const HairStyle& LivingStyle(const Boss& boss) {
    if (boss.frenzied) {
        return kFrenziedBossStyle;
    }
    return boss.phase == BossPhase::Two ? kEnragedBossStyle : kBossStyle;
}

// Every pose of the boss ends here: the glow of its eyes on the floor, the body, its shield around it, and
// stars while its broken shield staggers it.
void DrawBossBody(const Boss& boss, Vec2 position, float radius, const HairPose& pose, const Art& art) {
    const HairStyle* style = &LivingStyle(boss);
    if (boss.frenzied) {
        const float pulse = 0.5f + 0.5f * std::sin(pose.seconds * 8.0f);
        DrawCircleV(ToRaylib(position), radius * (1.0f + kFrenzyGlow), Fade(kBlood, 0.08f + 0.1f * pulse));
    }
    BeginBlendMode(BLEND_ADDITIVE);
    DrawGlow(art, position, radius * kEyeLightReach, Fade(style->eye, kEyeLight));
    EndBlendMode();
    DrawHairBody(position, radius, *style, pose, art);
    DrawBossShield(boss.shield, position, radius, pose.seconds);
    if (IsStaggered(boss)) {
        DrawStunStars(position, radius, boss.staggerSeconds);
    }
}

// Crouched and trembling through the warning, stretched along the run with ghosts behind it while it runs,
// and with stars over its head while it is stunned. The eyes look down its path instead of at the player.
void DrawChargingBoss(const Boss& boss, Vec2 position, HairPose pose, const Art& art) {
    const BossAttack& attack = *boss.attack;
    const ChargeStats& stats = boss.charge;
    float radius = boss.radius;
    pose.lookAt = position + attack.direction * boss.radius;
    pose.stretchAlong = attack.direction;
    switch (attack.stage) {
    case AttackStage::Warning: {
        const float warning = WarningSeconds(boss, attack);
        const float crouch = std::min(attack.stageSeconds / warning, 1.0f);
        radius *= 1.0f - kChargeCrouch * crouch;
        pose.stretch = 1.0f + (kCrouchSquash - 1.0f) * crouch;
        if (attack.stageSeconds > warning - stats.aimLockSeconds) {
            const Vec2 side{-attack.direction.y, attack.direction.x};
            position += side * (kChargeTremble * std::sin(attack.stageSeconds * 90.0f));
        }
        break;
    }
    case AttackStage::Active:
        pose.stretch = kRunStretch;
        OpenMouth(boss, pose, kChargeMouth);
        for (int i = kChargeGhosts; i >= 1; --i) {
            const Vec2 ghost = position - attack.direction * (radius * 0.5f * static_cast<float>(i));
            DrawCircleV(ToRaylib(ghost), radius * (1.0f - 0.1f * static_cast<float>(i)),
                        Fade(kBossFill, 0.35f / static_cast<float>(i)));
        }
        break;
    case AttackStage::Recovery:
        break;
    }
    pose.lookAt = position + attack.direction * boss.radius;
    DrawBossBody(boss, position, radius, pose, art);
    if (attack.stage == AttackStage::Recovery && attack.stunned) {
        DrawStunStars(position, radius, attack.stageSeconds);
    }
}

// Swells through the warning, deflates as it spits, mouth wide, and is back to its size while it recovers.
// It watches the player the whole time.
void DrawSpittingBoss(const Boss& boss, Vec2 position, HairPose pose, const Art& art) {
    const BossAttack& ring = *boss.attack;
    float radius = boss.radius;
    if (ring.stage == AttackStage::Warning) {
        radius *= 1.0f + kRingSwell * std::min(ring.stageSeconds / WarningSeconds(boss, ring), 1.0f);
    } else if (ring.stage == AttackStage::Active) {
        radius *= 1.0f - kRingSpitShrink;
        OpenMouth(boss, pose, kSpitMouth);
    }
    DrawBossBody(boss, position, radius, pose, art);
}

// Crouches, flies up in an arc toward its shadow, stretched and growing as it rises, and lands flattened.
// In the air it is drawn above everything else, since it is over the player's head.
void DrawLeapingBoss(const Boss& boss, Vec2 position, HairPose pose, const Art& art) {
    const BossAttack& leap = *boss.attack;
    float radius = boss.radius;
    pose.stretchAlong = Vec2{0.0f, 1.0f};
    switch (leap.stage) {
    case AttackStage::Warning: {
        const float crouch = LeapCrouchSeconds(boss, leap);
        const float airborne = leap.stageSeconds - crouch;
        if (airborne <= 0.0f) {
            const float down = leap.stageSeconds / crouch;
            radius *= 1.0f - kLeapCrouch * down;
            pose.stretch = 1.0f + (kCrouchSquash - 1.0f) * down;
            break;
        }
        const float air = std::min(airborne / LeapAirSeconds(boss, leap), 1.0f);
        const float arc = std::sin(std::numbers::pi_v<float> * air);
        position.y -= kLeapHeight * arc;
        radius *= 1.0f + kLeapGrow * arc;
        pose.stretch = 1.0f + (kAirStretch - 1.0f) * arc;
        break;
    }
    case AttackStage::Active:
        radius *= 1.0f + kLeapLandingSquash;
        pose.stretch = kLandingSquash;
        break;
    case AttackStage::Recovery:
        break;
    }
    DrawBossBody(boss, position, radius, pose, art);
}

// Leans back through the warning, and jolts toward the player for a moment with every burst.
void DrawVolleyingBoss(const Boss& boss, Vec2 position, HairPose pose, const Art& art) {
    const BossAttack& volley = *boss.attack;
    float radius = boss.radius;
    if (volley.stage == AttackStage::Warning) {
        radius *= 1.0f - kVolleyLean * std::min(volley.stageSeconds / WarningSeconds(boss, volley), 1.0f);
    } else if (volley.stage == AttackStage::Active) {
        const float sinceBurst = std::fmod(volley.stageSeconds, boss.volley.burstIntervalSeconds);
        if (sinceBurst < kVolleyBurstFlash) {
            radius *= 1.0f + kVolleyBurstSwell;
            pose.stretchAlong = volley.direction;
            pose.stretch = kBurstStretch;
            OpenMouth(boss, pose, kBurstMouth);
        }
    }
    DrawBossBody(boss, position, radius, pose, art);
}

// Swells and shudders while it howls, its mouth opening wider, and settles back as the hairballs come out.
void DrawSummoningBoss(const Boss& boss, Vec2 position, HairPose pose, const Art& art) {
    const BossAttack& summon = *boss.attack;
    float radius = boss.radius;
    if (summon.stage == AttackStage::Warning) {
        const float howl = std::min(summon.stageSeconds / WarningSeconds(boss, summon), 1.0f);
        radius *= 1.0f + kSummonSwell * howl;
        position.x += kSummonShudder * std::sin(summon.stageSeconds * 60.0f);
        OpenMouth(boss, pose, howl);
    }
    DrawBossBody(boss, position, radius, pose, art);
}

// Swells and trembles harder as the transformation goes on, grows a mouth that opens into a roar, and its
// eyes flicker from amber to red, faster toward the end. No shield is drawn: nothing can hurt it.
void DrawTransformingBoss(const Boss& boss, Vec2 position, HairPose pose, const Art& art) {
    const float total = boss.phaseTwo.transformSeconds;
    const float progress = std::clamp(1.0f - boss.transformSecondsLeft / total, 0.0f, 1.0f);
    const float seconds = total - boss.transformSecondsLeft;
    position.x += kTransformTremble * progress * std::sin(seconds * 70.0f);
    const float radius = boss.radius * (1.0f + kTransformSwell * progress);
    const bool red = std::sin(seconds * (6.0f + 30.0f * progress)) > 0.0f;
    pose.mouth = progress;
    pose.flashing = false;
    DrawHairBody(position, radius, red ? kEnragedBossStyle : kBossStyle, pose, art);
}

// The style of a living boss turned toward a dead one by `amount`, keeping its own number of locks, so the
// hair does not jump as it dies.
HairStyle Dying(const HairStyle& living, float amount) {
    return HairStyle{.fill = Mix(living.fill, kDeadBossStyle.fill, amount),
                     .edge = Mix(living.edge, kDeadBossStyle.edge, amount),
                     .sheen = Mix(living.sheen, kDeadBossStyle.sheen, amount),
                     .eye = Mix(living.eye, kDeadBossStyle.eye, amount),
                     .locks = living.locks,
                     .lockLength = living.lockLength + (kDeadBossStyle.lockLength - living.lockLength) * amount,
                     .bristle = living.bristle * (1.0f - amount),
                     .flash = living.flash,
                     .eyeSize = living.eyeSize};
}

// The death, `sinceDied` seconds in: the killing blow flashes it white, it convulses and swells with its
// mouth wide, and the colour drains out of it; then it sinks and its eyes shut. Past `kBossDeathSeconds` it
// lies still. Its children's hair bursts from it at the same moment (see the particles).
void DrawDyingBoss(const Boss& boss, Vec2 position, HairPose pose, float sinceDied, const Art& art) {
    const float progress = std::clamp(sinceDied / kBossDeathSeconds, 0.0f, 1.0f);
    const float convulsion = std::clamp(sinceDied / kDeathConvulsionSeconds, 0.0f, 1.0f);
    const float shaking = 1.0f - convulsion;
    position += Vec2{std::sin(sinceDied * 53.0f), std::cos(sinceDied * 47.0f)} * (kDeathShake * shaking);
    const float radius = boss.radius * (1.0f + kDeathSwell * std::sin(std::numbers::pi_v<float> * convulsion));

    const float grey = std::clamp((progress - kDeathGreyStart) / (1.0f - kDeathGreyStart), 0.0f, 1.0f);
    const float eased = grey * grey * (3.0f - 2.0f * grey);
    const HairStyle style = Dying(LivingStyle(boss), eased);
    pose.flashing = sinceDied < kDeathConvulsionSeconds * 0.2f;
    pose.mouth = eased < kDeathEyesShut ? 1.0f - eased / kDeathEyesShut : -1.0f;
    pose.dead = eased >= kDeathEyesShut;
    pose.stretchAlong = Vec2{0.0f, 1.0f};
    pose.stretch = 1.0f + (kDeathSag - 1.0f) * eased;

    BeginBlendMode(BLEND_ADDITIVE);
    DrawGlow(art, position, radius * kEyeLightReach, Fade(style.eye, kEyeLight * (1.0f - eased)));
    EndBlendMode();
    DrawHairBody(position, radius, style, pose, art);
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

void DrawBoss(const Boss& boss, Vec2 playerPosition, float seconds, float alpha, const Art& art, float sinceDied) {
    const Vec2 position = Lerp(boss.motion.previousPosition, boss.motion.position, alpha);
    const HairPose pose{.lookAt = playerPosition,
                        .light = playerPosition,
                        .seconds = seconds,
                        .mouth = RestingMouth(boss),
                        .flashing = boss.hitFlashSeconds > 0.0f};
    if (IsDead(boss.health)) {
        DrawDyingBoss(boss, position, pose, sinceDied, art);
        return;
    }
    if (IsTransforming(boss)) {
        DrawTransformingBoss(boss, position, pose, art);
        return;
    }
    if (!boss.attack) {
        DrawBossBody(boss, position, boss.radius, pose, art);
        return;
    }
    switch (boss.attack->kind) {
    case AttackKind::Charge:
        DrawChargingBoss(boss, position, pose, art);
        break;
    case AttackKind::HairRing:
        DrawSpittingBoss(boss, position, pose, art);
        break;
    case AttackKind::Leap:
        DrawLeapingBoss(boss, position, pose, art);
        break;
    case AttackKind::Volley:
        DrawVolleyingBoss(boss, position, pose, art);
        break;
    case AttackKind::Summon:
        DrawSummoningBoss(boss, position, pose, art);
        break;
    }
}

} // namespace horde
