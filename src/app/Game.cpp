#include "app/Game.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <random>
#include <string>

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"

namespace horde {

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

// At zoom 1 one world unit is one pixel, so the view is the screen size.
constexpr Vec2 kViewSize{static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight)};
constexpr float kGridSpacing = 80.0f;
constexpr float kWallThickness = 4.0f;
constexpr float kPlayerOutline = 3.0f;
// The eye sits toward the facing direction, so the player reads as pointing somewhere.
constexpr float kEyeOffset = 0.55f;
constexpr float kEyeRadius = 0.28f;
// Hairballs and the boss: a round body with short tufts around it, and two eyes that look at the player.
// Sizes are fractions of the body radius, so the boss is the same drawing, scaled up.
constexpr float kTuftLength = 0.35f;
constexpr float kHairballEyeOffset = 0.45f;
constexpr float kHairballEyeSpread = 0.35f;
constexpr float kHairballEyeRadius = 0.22f;
// The boss's health bar runs along the bottom of the screen, under its name, above the controls line.
constexpr const char* kBossName = "The Matriarch";
constexpr int kBossBarWidth = 640;
constexpr int kBossBarHeight = 12;
constexpr int kBossBarY = kScreenHeight - 96;
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
// Barrier breathes, so it reads as alive and growing back. The shield's own bar runs under the boss's.
constexpr float kShieldGap = 6.0f;
constexpr float kShieldWidth = 5.0f;
constexpr float kShieldBounceWidth = 4.0f;
constexpr float kBarrierPulse = 6.0f;
constexpr int kShieldBarHeight = 6;
// The Hair Ring. During the warning the boss swells by this fraction and the ring is drawn on the floor
// just outside the tips of its tufts, with the gap as a pale lane running out from it. As it spits, it
// deflates for a moment. A strand is a short streak behind a round tip.
constexpr float kRingSwell = 0.15f;
constexpr float kRingSpitShrink = 0.08f;
constexpr float kRingMarkGap = 8.0f;
constexpr float kRingMarkWidth = 10.0f;
constexpr float kRingLaneLength = 420.0f;
constexpr float kStrandTail = 12.0f;
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
// The Summon. The boss swells and shudders as it howls. Each mark is a dark pit on the floor with a red
// rim that closes in on it as the hairball's moment comes.
constexpr float kSummonSwell = 0.12f;
constexpr float kSummonShudder = 2.0f;
constexpr float kMarkerRadius = 18.0f;
constexpr float kMarkerRimStart = 2.5f;
// The transformation: the body swells and trembles harder as it goes, and the eyes flicker from amber to
// red, faster toward the end.
constexpr float kTransformSwell = 0.2f;
constexpr float kTransformTremble = 4.0f;
// While invulnerable the player blinks: faded for one period, solid for the next.
constexpr float kBlinkPeriod = 0.1f;
constexpr float kBlinkFade = 0.35f;
// The gun is a barrel that sticks out of the body toward the aim, as long as `WeaponStats::barrelLength`.
// The SMG's is thicker, so it reads as the bigger gun.
constexpr float kPistolBarrelWidth = 6.0f;
constexpr float kSmgBarrelWidth = 9.0f;
constexpr float kMuzzleFlashRadius = 8.0f;
// The dash leaves a fading trail of ghosts between where it started and the body, so a move that takes
// seven frames still reads as a jump.
constexpr int kDashGhosts = 4;
// The dash bar sits under the health bar.
constexpr int kDashBarY = 64;
constexpr int kDashBarHeight = 6;
// Flask icons sit to the right of the health bar; a drink shows as a ring filling around the player.
constexpr int kFlaskIconX = 282;
constexpr int kFlaskIconWidth = 12;
constexpr int kFlaskIconHeight = 18;
constexpr int kFlaskIconGap = 6;
constexpr float kDrinkRingGap = 4.0f;
constexpr float kDrinkRingWidth = 4.0f;
// A bullet is drawn as a short streak behind its head, so its direction reads even in a still frame.
constexpr float kBulletStreak = 14.0f;
constexpr float kBulletWidth = 4.0f;
// Weapon panel in the bottom right corner, one box per slot.
constexpr int kSlotBoxWidth = 150;
constexpr int kSlotBoxHeight = 48;
constexpr int kSlotBoxGap = 8;
constexpr int kHudMargin = 12;
// The end screens: a dark band across the middle of the screen, with soft top and bottom edges.
constexpr int kEndBandTop = kScreenHeight / 2 - 120;
constexpr int kEndBandHeight = 220;
constexpr int kEndBandFeather = 48;

constexpr Color kBackground{20, 18, 24, 255};
constexpr Color kFloor{34, 30, 40, 255};
constexpr Color kGrid{44, 40, 52, 255};
constexpr Color kWall{110, 96, 120, 255};
constexpr Color kPlayerFill{90, 190, 230, 255};
constexpr Color kPlayerEdge{20, 60, 80, 255};
constexpr Color kPlayerDeadFill{95, 95, 105, 255};
constexpr Color kPlayerDeadEdge{45, 45, 52, 255};
constexpr Color kHairballFill{150, 92, 70, 255};
constexpr Color kHairballEdge{70, 40, 32, 255};
constexpr Color kHairballEye{235, 225, 200, 255};
// The boss is darker and redder than its children, with eyes that glow.
constexpr Color kBossFill{112, 58, 64, 255};
constexpr Color kBossEdge{48, 24, 30, 255};
constexpr Color kBossEye{255, 190, 90, 255};
constexpr Color kBossBar{170, 32, 40, 255};
constexpr Color kSoftText{170, 165, 180, 255};
constexpr Color kBlood{220, 60, 50, 255};
constexpr Color kVictoryGold{230, 185, 95, 255};
constexpr Color kStunStar{255, 225, 130, 255};
// Paler and redder than the boss's body, so a strand reads against the dark floor and as something that
// hurts. The gap lane is the one pale, safe thing in the warning.
constexpr Color kStrand{225, 130, 110, 255};
constexpr Color kShell{95, 155, 255, 255};
constexpr Color kBarrier{255, 150, 50, 255};
constexpr Color kExposed{235, 235, 240, 255};
constexpr Color kRingLane{205, 205, 220, 255};
// The shockwave is dust thrown up by the landing, red where it still hurts.
constexpr Color kShockwave{230, 120, 95, 255};
constexpr Color kBossEyeEnraged{255, 60, 50, 255};
constexpr Color kFrenzyFill{135, 42, 50, 255};
constexpr Color kBarBack{50, 20, 24, 255};
constexpr Color kBarHurt{255, 140, 120, 255};
constexpr Color kBarEdge{120, 100, 110, 255};
// Light steel, so the barrel stands out against the dark floor where it leaves the body.
constexpr Color kGun{175, 180, 195, 255};
constexpr Color kMuzzleFlash{255, 220, 120, 255};
constexpr Color kCrosshair{240, 240, 245, 255};
constexpr Color kBullet{255, 230, 140, 255};
constexpr Color kFlask{245, 150, 45, 255};
constexpr Color kHitFlash{255, 255, 255, 255};

// How a hairy body is drawn. `flash` is how far a hit turns it toward white: all the way for a hairball,
// which dies in a few hits, but only part of the way for the boss, which the SMG hits ten times a second
// and which would otherwise stay white for the whole fight.
struct HairStyle {
    Color fill;
    Color edge;
    Color eye;
    int tufts;
    float tuftWidth;
    float outline;
    float flash;
};

constexpr HairStyle kHairballStyle{kHairballFill, kHairballEdge, kHairballEye, 12, 3.0f, 3.0f, 1.0f};
constexpr HairStyle kSpittingHairballStyle{kHairballFill, kHairballEdge, kBlood, 12, 3.0f, 3.0f, 1.0f};
constexpr float kHairballSpitSwell = 0.2f;
constexpr HairStyle kBossStyle{kBossFill, kBossEdge, kBossEye, 28, 6.0f, 5.0f, 0.45f};
// In the second phase the eyes turn red, so the change reads at a glance.
constexpr HairStyle kEnragedBossStyle{kBossFill, kBossEdge, kBossEyeEnraged, 28, 6.0f, 5.0f, 0.45f};
// In a frenzy it bristles (more tufts) and reddens, over a pulsing glow on the floor.
constexpr HairStyle kFrenziedBossStyle{kFrenzyFill, kBossEdge, kBossEyeEnraged, 40, 6.0f, 5.0f, 0.45f};
constexpr float kFrenzyGlow = 0.7f;
// A dead boss turns grey like a dead player, and its eyes go dark.
constexpr HairStyle kDeadBossStyle{kPlayerDeadFill, kPlayerDeadEdge, kPlayerDeadEdge, 28, 6.0f, 5.0f, 0.0f};

#ifndef NDEBUG
constexpr const char* kControlsHint =
    "WASD move  Mouse aim  Click fire  Space dash  E heal  Q/1/2 swap gun  Esc quit  Debug: F1 F2 F3 F4";
#elif defined(__EMSCRIPTEN__)
// In a browser the tab is closed, not the game: Esc only gives the mouse back to the page.
constexpr const char* kControlsHint = "WASD move  Mouse aim  Click fire  Space dash  E heal  Q/1/2/wheel swap gun";
#else
constexpr const char* kControlsHint =
    "WASD move  Mouse aim  Click fire  Space dash  E heal  Q/1/2/wheel swap gun  Esc quit";
#endif

const char* FireModeLabel(FireMode mode) {
    return mode == FireMode::Auto ? "AUTO" : "SEMI-AUTO";
}

void DrawCenteredText(const char* text, int y, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, (kScreenWidth - width) / 2, y, fontSize, color);
}

void DrawArena(const Arena& arena) {
    const Vec2 size = arena.max - arena.min;
    DrawRectangleV(ToRaylib(arena.min), ToRaylib(size), kFloor);

    // The grid gives the eye something fixed to judge speed against.
    for (float x = arena.min.x + kGridSpacing; x < arena.max.x; x += kGridSpacing) {
        DrawLineV(ToRaylib({x, arena.min.y}), ToRaylib({x, arena.max.y}), kGrid);
    }
    for (float y = arena.min.y + kGridSpacing; y < arena.max.y; y += kGridSpacing) {
        DrawLineV(ToRaylib({arena.min.x, y}), ToRaylib({arena.max.x, y}), kGrid);
    }

    // raylib draws the outline inward, so the rectangle grows by the wall thickness. Otherwise a mover
    // confined to the arena would overlap the wall.
    const Rectangle walls{arena.min.x - kWallThickness, arena.min.y - kWallThickness,
                          size.x + 2.0f * kWallThickness, size.y + 2.0f * kWallThickness};
    DrawRectangleLinesEx(walls, kWallThickness, kWall);
}

// Ghosts fade with the dash's invulnerability, so the trail also shows how long the player is safe.
void DrawDashTrail(const Player& player, Vec2 position) {
    const Dash& dash = player.dash;
    if (!IsDashInvulnerable(dash)) {
        return;
    }
    const float strength = dash.invulnerableSeconds / dash.stats.invulnerability;
    for (int i = 0; i < kDashGhosts; ++i) {
        const float along = static_cast<float>(i) / kDashGhosts;
        const Vec2 ghost = Lerp(dash.origin, position, along);
        const float size = player.radius * (0.6f + 0.4f * along);
        DrawCircleV(ToRaylib(ghost), size, Fade(kPlayerFill, 0.4f * strength * along));
    }
}

// A ring that fills clockwise while the player drinks, so both the player and anyone watching can see
// how long they are exposed.
void DrawDrinkRing(const Player& player, Vec2 position) {
    const Flasks& flasks = player.flasks;
    if (!IsDrinking(flasks)) {
        return;
    }
    const float progress = 1.0f - flasks.drinkingSeconds / flasks.stats.drinkSeconds;
    const float inner = player.radius + kDrinkRingGap;
    DrawRing(ToRaylib(position), inner, inner + kDrinkRingWidth, -90.0f, -90.0f + 360.0f * progress, 32, kFlask);
}

void DrawPlayer(const Player& player, float alpha) {
    // Drawn between the last two fixed steps, so movement is smooth above 60 Hz.
    const Vec2 position = Lerp(player.motion.previousPosition, player.motion.position, alpha);
    const Health& health = player.health;
    DrawDashTrail(player, position);
    DrawDrinkRing(player, position);

    Color edge = IsDead(health) ? kPlayerDeadEdge : kPlayerEdge;
    Color fill = IsDead(health) ? kPlayerDeadFill : kPlayerFill;
    Color gun = IsDead(health) ? kPlayerDeadEdge : kGun;
    // The blink tells the player they were hit and that the next touch will not hurt yet.
    const bool fadedHalf = static_cast<int>(health.invulnerableSeconds / kBlinkPeriod) % 2 == 1;
    if (IsInvulnerable(health) && fadedHalf) {
        edge = Fade(edge, kBlinkFade);
        fill = Fade(fill, kBlinkFade);
        gun = Fade(gun, kBlinkFade);
    }

    // The barrel goes under the body, so it reads as held, sticking out toward the aim.
    const Weapon& weapon = Equipped(player.loadout);
    const bool automatic = weapon.stats.mode == FireMode::Auto;
    const Vec2 muzzle = MuzzlePoint(position, player.facing, player.radius, weapon.stats);
    DrawLineEx(ToRaylib(position), ToRaylib(muzzle), automatic ? kSmgBarrelWidth : kPistolBarrelWidth, gun);

    DrawCircleV(ToRaylib(position), player.radius, edge);
    DrawCircleV(ToRaylib(position), player.radius - kPlayerOutline, fill);
    DrawCircleV(ToRaylib(position + player.facing * (player.radius * kEyeOffset)), player.radius * kEyeRadius, edge);

    if (weapon.muzzleFlashSeconds > 0.0f) {
        DrawCircleV(ToRaylib(muzzle), kMuzzleFlashRadius, kMuzzleFlash);
    }
}

Color Mix(Color from, Color to, float amount) {
    const auto channel = [amount](unsigned char a, unsigned char b) {
        const float mixed = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * amount;
        return static_cast<unsigned char>(mixed);
    };
    return Color{channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), channel(from.a, to.a)};
}

void DrawHairBody(Vec2 position, float radius, Vec2 lookAt, const HairStyle& style, bool flashing) {
    // A hit turns the body toward white for a moment, so every bullet that lands is visible.
    const float flash = flashing ? style.flash : 0.0f;
    const Color edge = Mix(style.edge, kHitFlash, flash);
    const Color fill = Mix(style.fill, kHitFlash, flash);

    for (int i = 0; i < style.tufts; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(style.tufts);
        const Vec2 out{std::cos(angle), std::sin(angle)};
        const Vec2 root = position + out * (radius * 0.7f);
        const Vec2 tip = position + out * (radius * (1.0f + kTuftLength));
        DrawLineEx(ToRaylib(root), ToRaylib(tip), style.tuftWidth, edge);
    }
    DrawCircleV(ToRaylib(position), radius, edge);
    DrawCircleV(ToRaylib(position), radius - style.outline, fill);

    // The eyes look at the player, so it is clear what the body is after even when it stands still.
    const Vec2 look = Normalized(lookAt - position);
    const Vec2 side{-look.y, look.x};
    const Vec2 eyes = position + look * (radius * kHairballEyeOffset);
    DrawCircleV(ToRaylib(eyes + side * (radius * kHairballEyeSpread)), radius * kHairballEyeRadius, style.eye);
    DrawCircleV(ToRaylib(eyes - side * (radius * kHairballEyeSpread)), radius * kHairballEyeRadius, style.eye);
}

// A hairball about to spit swells and reddens its eyes, so the player can see which one is next.
void DrawHairball(const Enemy& enemy, Vec2 playerPosition, float spitWarningSeconds, float alpha) {
    const Vec2 position = Lerp(enemy.motion.previousPosition, enemy.motion.position, alpha);
    const bool aboutToSpit = enemy.spits && enemy.secondsUntilSpit <= spitWarningSeconds;
    const float radius = enemy.radius * (aboutToSpit ? 1.0f + kHairballSpitSwell : 1.0f);
    DrawHairBody(position, radius, playerPosition, aboutToSpit ? kSpittingHairballStyle : kHairballStyle,
                 enemy.hitFlashSeconds > 0.0f);
}

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

Color ShieldColor(ShieldKind kind) {
    return kind == ShieldKind::Shell ? kShell : kBarrier;
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

// A dark pit the size of a hairball, with a red rim that closes in on it: when the rim reaches the pit, the
// hairball comes out.
void DrawSpawnMarker(const SpawnMarker& marker) {
    const float left = std::clamp(marker.secondsLeft / marker.totalSeconds, 0.0f, 1.0f);
    const Vector2 center = ToRaylib(marker.position);
    DrawCircleV(center, kMarkerRadius, Fade(BLACK, 0.45f));
    const float rim = kMarkerRadius * (1.0f + (kMarkerRimStart - 1.0f) * left);
    DrawRing(center, rim - 2.0f, rim, 0.0f, 360.0f, 32, Fade(kBlood, 0.9f - 0.5f * left));
}

// The band that hurts, fading as the ring spreads out. A wave that already hit the player is drawn faint,
// so it is clear it cannot hurt again.
void DrawShockwave(const Shockwave& wave) {
    const ShockwaveStats& stats = wave.stats;
    const float left = 1.0f - std::min(wave.radius / stats.reach, 1.0f);
    const float inner = std::max(wave.radius - stats.thickness, 0.0f);
    DrawRing(ToRaylib(wave.center), inner, wave.radius, 0.0f, 360.0f, 96,
             Fade(kShockwave, (wave.spent ? 0.25f : 0.9f) * (0.3f + 0.7f * left)));
}

void DrawBullet(const Projectile& bullet, float alpha) {
    const Vec2 head = Lerp(bullet.motion.previousPosition, bullet.motion.position, alpha);
    const Vec2 tail = head - Normalized(bullet.motion.velocity) * kBulletStreak;
    DrawLineEx(ToRaylib(tail), ToRaylib(head), kBulletWidth, kBullet);
    DrawCircleV(ToRaylib(head), bullet.radius, kBullet);
}

// A dark streak behind a pale tip, as wide as the strand's hit circle, so what the player sees is what can
// hurt them.
void DrawHairStrand(const Projectile& strand, float alpha) {
    const Vec2 head = Lerp(strand.motion.previousPosition, strand.motion.position, alpha);
    const Vec2 tail = head - Normalized(strand.motion.velocity) * kStrandTail;
    DrawLineEx(ToRaylib(tail), ToRaylib(head), strand.radius * 1.2f, kBossEdge);
    DrawCircleV(ToRaylib(head), strand.radius, kBossEdge);
    DrawCircleV(ToRaylib(head), strand.radius - 3.0f, kStrand);
}

void DrawHealthBar(const Health& health) {
    constexpr int kX = 10;
    constexpr int kY = 38;
    constexpr int kWidth = 260;
    constexpr int kHeight = 22;
    const float fraction = static_cast<float>(health.current) / static_cast<float>(health.max);

    DrawRectangle(kX, kY, kWidth, kHeight, kBarBack);
    // The fill turns pale while the player is invulnerable, so a hit reads on the bar as well as on the body.
    DrawRectangle(kX, kY, static_cast<int>(kWidth * fraction), kHeight, IsInvulnerable(health) ? kBarHurt : kBlood);
    DrawRectangleLines(kX, kY, kWidth, kHeight, kBarEdge);
    DrawText(TextFormat("%d / %d", health.current, health.max), kX + 8, kY + 2, 18, RAYWHITE);
}

// Under the boss's bar: what is left of the shield, in its colour, with its name at the right end above the
// bar. Once it breaks, a white strip drains away with the exposed time, so the player can see how long the
// opening lasts.
void DrawShieldBar(const Shield& shield, int x) {
    const int y = kBossBarY + kBossBarHeight + 3;
    float fraction = shield.exposedSecondsLeft / shield.stats.exposedSeconds;
    Color color = kExposed;
    const char* label = "EXPOSED";
    if (IsShieldUp(shield)) {
        fraction = shield.strength / MaxStrength(shield.stats, shield.kind);
        color = ShieldColor(shield.kind);
        label = shield.kind == ShieldKind::Shell ? "SHELL" : "BARRIER";
    }
    DrawRectangle(x, y, kBossBarWidth, kShieldBarHeight, kBarBack);
    DrawRectangle(x, y, static_cast<int>(kBossBarWidth * std::clamp(fraction, 0.0f, 1.0f)), kShieldBarHeight, color);
    DrawText(label, x + kBossBarWidth - MeasureText(label, 20), kBossBarY - 24, 20, color);
}

// Wide and thin along the bottom of the screen, with the name above it, so it reads as the one fight that
// matters and stays out of the way of the arena.
void DrawBossBar(const Boss& boss) {
    constexpr int kX = (kScreenWidth - kBossBarWidth) / 2;
    float fraction = static_cast<float>(boss.health.current) / static_cast<float>(boss.health.max);
    // While it transforms, the bar fills back up over the transformation.
    if (IsTransforming(boss)) {
        fraction = 1.0f - boss.transformSecondsLeft / boss.phaseTwo.transformSeconds;
    }
    DrawText(kBossName, kX, kBossBarY - 24, 20, RAYWHITE);
    DrawRectangle(kX, kBossBarY, kBossBarWidth, kBossBarHeight, kBarBack);
    DrawRectangle(kX, kBossBarY, static_cast<int>(kBossBarWidth * fraction), kBossBarHeight, kBossBar);
    DrawRectangleLines(kX, kBossBarY, kBossBarWidth, kBossBarHeight, kBarEdge);
    if (!IsDead(boss.health) && !IsTransforming(boss)) {
        DrawShieldBar(boss.shield, kX);
    }
}

// One box per slot. The gun in hand is lit, so the player can tell at a glance which one they hold.
void DrawWeaponPanel(const Loadout& loadout) {
    const int top = kScreenHeight - kSlotBoxHeight - kHudMargin;
    for (int slot = 0; slot < kWeaponSlots; ++slot) {
        const Weapon& weapon = loadout.slots[static_cast<std::size_t>(slot)];
        const bool inHand = slot == loadout.equipped;
        const int slotsToTheRight = kWeaponSlots - slot;
        const int left = kScreenWidth - kHudMargin - slotsToTheRight * (kSlotBoxWidth + kSlotBoxGap) + kSlotBoxGap;
        const Rectangle box{static_cast<float>(left), static_cast<float>(top), static_cast<float>(kSlotBoxWidth),
                            static_cast<float>(kSlotBoxHeight)};

        DrawRectangleRec(box, inHand ? Fade(kPlayerFill, 0.25f) : Fade(BLACK, 0.45f));
        DrawRectangleLinesEx(box, inHand ? 2.0f : 1.0f, inHand ? kPlayerFill : kBarEdge);
        const Color text = inHand ? RAYWHITE : kSoftText;
        const std::string name = TextToUpper(std::string(weapon.stats.name).c_str());
        DrawText(TextFormat("%d  %s", slot + 1, name.c_str()), left + 10, top + 6, 20, text);
        DrawText(FireModeLabel(weapon.stats.mode), left + 10, top + 28, 14, text);
    }
}

// Drawn in place of the system cursor while playing, so the aim point is easy to find in a crowd.
void DrawCrosshair(Vector2 at) {
    constexpr float kRadius = 10.0f;
    constexpr float kTick = 6.0f;
    DrawCircleLinesV(at, kRadius, kCrosshair);
    DrawLineEx({at.x - kRadius - kTick, at.y}, {at.x - kRadius + 3.0f, at.y}, 2.0f, kCrosshair);
    DrawLineEx({at.x + kRadius - 3.0f, at.y}, {at.x + kRadius + kTick, at.y}, 2.0f, kCrosshair);
    DrawLineEx({at.x, at.y - kRadius - kTick}, {at.x, at.y - kRadius + 3.0f}, 2.0f, kCrosshair);
    DrawLineEx({at.x, at.y + kRadius - 3.0f}, {at.x, at.y + kRadius + kTick}, 2.0f, kCrosshair);
    DrawCircleV(at, 1.5f, kCrosshair);
}

// One bottle per charge, lit while it is full.
void DrawFlaskIcons(const Flasks& flasks) {
    for (int i = 0; i < flasks.stats.maxCharges; ++i) {
        const float x = static_cast<float>(kFlaskIconX + i * (kFlaskIconWidth + kFlaskIconGap));
        const Rectangle bottle{x, 41.0f, static_cast<float>(kFlaskIconWidth), static_cast<float>(kFlaskIconHeight)};
        const Rectangle neck{x + 3.0f, 37.0f, static_cast<float>(kFlaskIconWidth) - 6.0f, 5.0f};
        const Color fill = i < flasks.charges ? kFlask : kBarBack;
        DrawRectangleRec(neck, kBarEdge);
        DrawRectangleRounded(bottle, 0.5f, 6, fill);
        DrawRectangleRoundedLinesEx(bottle, 0.5f, 6, 1.0f, kBarEdge);
    }
}

// Fills up as the dash recharges, and lights up when it is ready.
void DrawDashBar(const Dash& dash) {
    constexpr int kX = 10;
    constexpr int kWidth = 260;
    const float total = dash.stats.duration + dash.stats.cooldown;
    const float ready = 1.0f - dash.cooldownSeconds / total;
    DrawRectangle(kX, kDashBarY, kWidth, kDashBarHeight, kBarBack);
    DrawRectangle(kX, kDashBarY, static_cast<int>(kWidth * ready), kDashBarHeight,
                  dash.cooldownSeconds > 0.0f ? kSoftText : kPlayerFill);
}

// What killed the player, for the death screen: knowing which attack did it is how the fight is learned.
const char* DeathCause(std::optional<DamageSource> source) {
    if (!source) {
        return "Slain";
    }
    switch (*source) {
    case DamageSource::BossTouch:
        return "Slain by her touch";
    case DamageSource::Charge:
        return "Slain by her Charge";
    case DamageSource::HairRing:
        return "Slain by her Hair Ring";
    case DamageSource::Volley:
        return "Slain by her Volley";
    case DamageSource::LeapImpact:
        return "Slain by her Leap";
    case DamageSource::Shockwave:
        return "Slain by her shockwave";
    case DamageSource::Frenzy:
        return "Slain by her frenzy";
    case DamageSource::Hairball:
        return "Slain by a hairball";
    }
    return "Slain";
}

// Minutes and seconds, the way the length of a fight is read.
std::string FormatDuration(float seconds) {
    const int whole = static_cast<int>(seconds);
    return TextFormat("%d:%02d", whole / 60, whole % 60);
}

// Rounded up, so a boss with any health left never reads 0%.
int HealthPercent(const Health& health) {
    return (health.current * 100 + health.max - 1) / health.max;
}

// The end of a fight, in the style of the games it borrows from: the world dims and a dark band across
// the middle carries one big word. It all fades in over the restart delay, so the moment the prompt lights
// up is the moment a press works.
void DrawEndScreen(const Session& session) {
    const float shown = std::min(session.stateSeconds / kRestartDelaySeconds, 1.0f);
    const bool won = session.state == SessionState::Victory;
    const World& world = session.world;

    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.35f * shown));
    const Color band = Fade(BLACK, 0.75f * shown);
    const Color clear = Fade(BLACK, 0.0f);
    DrawRectangleGradientV(0, kEndBandTop - kEndBandFeather, kScreenWidth, kEndBandFeather, clear, band);
    DrawRectangle(0, kEndBandTop, kScreenWidth, kEndBandHeight, band);
    DrawRectangleGradientV(0, kEndBandTop + kEndBandHeight, kScreenWidth, kEndBandFeather, band, clear);

    DrawCenteredText(won ? "VICTORY" : "YOU DIED", kEndBandTop + 28, 80, Fade(won ? kVictoryGold : kBlood, shown));

    const std::string duration = FormatDuration(world.elapsedSeconds);
    std::string summary;
    std::string detail;
    if (won) {
        const int accuracy = static_cast<int>(std::lround(Accuracy(world) * 100.0f));
        summary = TextFormat("You defeated %s on attempt %d, in %s.", kBossName, session.attempt, duration.c_str());
        detail = TextFormat("%d of %d shots on target (%d%%)", world.shotsHit, world.shotsFired, accuracy);
    } else {
        summary =
            TextFormat("Attempt %d. %s after %s.", session.attempt, DeathCause(world.lastHitBy), duration.c_str());
        detail = TextFormat("%s kept %d%% of her health.", kBossName, HealthPercent(world.boss.health));
    }
    DrawCenteredText(summary.c_str(), kEndBandTop + 124, 26, Fade(RAYWHITE, shown));
    DrawCenteredText(detail.c_str(), kEndBandTop + 154, 22, Fade(kSoftText, shown));

    // Dimmed until the restart delay has passed, so it is clear why an early press did nothing.
    const Color prompt = CanRestart(session) ? RAYWHITE : Fade(kSoftText, 0.4f * shown);
    const char* again = won ? "Press Enter or R (A or Start on a gamepad) to fight her again"
                            : "Press Enter or R (A or Start on a gamepad) to try again";
    // Inside the band, so it never lands on top of the player, who stands in the middle of the screen.
    DrawCenteredText(again, kEndBandTop + 192, 22, prompt);
}

// Under the version, in the top right corner, so the count of tries is always in sight.
void DrawAttempt(int attempt) {
    const char* text = TextFormat("Attempt %d", attempt);
    DrawText(text, kScreenWidth - kHudMargin - MeasureText(text, 20), 36, 20, kSoftText);
}

Camera2D ToRaylibCamera(const FollowCamera& camera, float alpha) {
    // The camera is interpolated like the player. If only the player were, the two would drift apart
    // by a fraction of a step every frame and the player would shimmer against the floor.
    const Vec2 center = Lerp(camera.previousCenter, camera.center, alpha);
    return Camera2D{.offset = {kScreenWidth * 0.5f, kScreenHeight * 0.5f},
                    .target = ToRaylib(center),
                    .rotation = 0.0f,
                    .zoom = 1.0f};
}

#ifndef NDEBUG
// Debug builds only. Drawn in world space: velocity in green (a quarter second ahead), facing in yellow,
// and a cross where the camera is centred.
void DrawDebugWorld(const Player& player, const FollowCamera& camera, float alpha) {
    const Motion& motion = player.motion;
    const Vec2 position = Lerp(motion.previousPosition, motion.position, alpha);
    DrawLineEx(ToRaylib(position), ToRaylib(position + motion.velocity * 0.25f), 2.0f, GREEN);
    DrawLineEx(ToRaylib(position), ToRaylib(position + player.facing * (player.radius * 2.0f)), 2.0f, YELLOW);

    const Vec2 center = Lerp(camera.previousCenter, camera.center, alpha);
    DrawLineEx(ToRaylib(center + Vec2{-8.0f, 0.0f}), ToRaylib(center + Vec2{8.0f, 0.0f}), 2.0f, ORANGE);
    DrawLineEx(ToRaylib(center + Vec2{0.0f, -8.0f}), ToRaylib(center + Vec2{0.0f, 8.0f}), 2.0f, ORANGE);
}

const char* AttackKindLabel(AttackKind kind) {
    switch (kind) {
    case AttackKind::Charge:
        return "charge";
    case AttackKind::HairRing:
        return "ring";
    case AttackKind::Leap:
        return "leap";
    case AttackKind::Volley:
        return "volley";
    case AttackKind::Summon:
        return "summon";
    }
    return "";
}

const char* AttackStageLabel(AttackStage stage) {
    switch (stage) {
    case AttackStage::Warning:
        return "warning";
    case AttackStage::Active:
        return "active";
    case AttackStage::Recovery:
        return "recovery";
    }
    return "";
}

// What the boss is doing, and for how long, so attack timings can be tuned against numbers.
const char* BossStateLabel(const Boss& boss) {
    if (IsDead(boss.health)) {
        return "dead";
    }
    if (IsTransforming(boss)) {
        return TextFormat("transforming %.2f", boss.transformSecondsLeft);
    }
    const char* phase = boss.phase == BossPhase::One ? "p1" : (boss.frenzied ? "p2 frenzy" : "p2");
    if (!boss.attack) {
        return TextFormat("%s stalk, attack in %.2f", phase, boss.secondsUntilAttack);
    }
    const BossAttack& attack = *boss.attack;
    const char* combo = attack.followUp ? " combo 1/2" : (attack.endsCombo ? " combo 2/2" : "");
    return TextFormat("%s %s %s %.2f/%.2f%s%s%s", phase, AttackKindLabel(attack.kind), AttackStageLabel(attack.stage),
                      attack.stageSeconds, WarningSeconds(boss, attack), attack.stunned ? " stunned" : "",
                      IsAirborne(boss) ? " airborne" : "", combo);
}

// The shield's kind and strength, or how long the boss stays exposed, and the stagger.
const char* ShieldLabel(const Boss& boss) {
    const Shield& shield = boss.shield;
    if (!IsShieldUp(shield)) {
        return TextFormat("shield exposed %.2f  stagger %.2f", shield.exposedSecondsLeft,
                          std::max(boss.staggerSeconds, 0.0f));
    }
    return TextFormat("shield %s %.0f / %.0f", shield.kind == ShieldKind::Shell ? "shell" : "barrier",
                      shield.strength, MaxStrength(shield.stats, shield.kind));
}

// How likely each attack is at the player's distance right now, and the one the next pick skips.
const char* AttackPickLabel(const World& world) {
    const Boss& boss = world.boss;
    const float distance = Length(world.player.motion.position - boss.motion.position);
    const AttackChoiceStats& choice = boss.choice;
    return TextFormat("pick %3.0f c%.1f r%.1f l%.1f v%.1f s%.1f not %s", distance,
                      AttackWeightAt(choice, AttackKind::Charge, distance),
                      AttackWeightAt(choice, AttackKind::HairRing, distance),
                      AttackWeightAt(choice, AttackKind::Leap, distance),
                      AttackWeightAt(choice, AttackKind::Volley, distance),
                      AttackWeightAt(choice, AttackKind::Summon, distance),
                      boss.lastAttack ? AttackKindLabel(*boss.lastAttack) : "-");
}

// Debug builds only, in screen space. The numbers I tune against: what the input asks for, what the
// body does, where the camera is, how many enemies and projectiles there are, the player's health window,
// and what the boss is doing.
void DrawDebugHud(const World& world, Vec2 moveInput) {
    const Player& player = world.player;
    const FollowCamera& camera = world.camera;
    const Motion& motion = player.motion;
    constexpr int kX = 10;
    constexpr int kY = 80; // Below the health and dash bars.
    constexpr int kLine = 20;
    DrawRectangle(kX - 4, kY - 4, 420, 17 * kLine + 8, Fade(BLACK, 0.6f));
    DrawText(TextFormat("input  %5.2f %5.2f  len %4.2f", moveInput.x, moveInput.y, Length(moveInput)), kX, kY,
             18, RAYWHITE);
    DrawText(TextFormat("speed  %5.0f / %.0f", Length(motion.velocity), player.stats.maxSpeed), kX, kY + kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("vel    %5.0f %5.0f", motion.velocity.x, motion.velocity.y), kX, kY + 2 * kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("facing %5.2f %5.2f", player.facing.x, player.facing.y), kX, kY + 3 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("pos    %5.0f %5.0f", motion.position.x, motion.position.y), kX, kY + 4 * kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("camera %5.0f %5.0f", camera.center.x, camera.center.y), kX, kY + 5 * kLine, 18, ORANGE);
    DrawText(TextFormat("enemies %d  bullets %d  hair %d", static_cast<int>(world.enemies.Size()),
                        static_cast<int>(world.bullets.Size()), static_cast<int>(world.hostileProjectiles.Size())),
             kX, kY + 6 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("health %d / %d  inv %.2f", player.health.current, player.health.max,
                        player.health.invulnerableSeconds),
             kX, kY + 7 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("time   %.2f s", world.elapsedSeconds), kX, kY + 8 * kLine, 18, RAYWHITE);
    const Weapon& weapon = Equipped(player.loadout);
    DrawText(TextFormat("weapon %s  cd %.2f", std::string(weapon.stats.name).c_str(), weapon.cooldownSeconds), kX,
             kY + 9 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("shots  %d hit %d  boss %d / %d", world.shotsFired, world.shotsHit, world.boss.health.current,
                        world.boss.health.max),
             kX, kY + 10 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("dash   cd %.2f  inv %.2f", player.dash.cooldownSeconds, player.dash.invulnerableSeconds), kX,
             kY + 11 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("flasks %d / %d  drink %.2f", player.flasks.charges, player.flasks.stats.maxCharges,
                        player.flasks.drinkingSeconds),
             kX, kY + 12 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("boss   %s", BossStateLabel(world.boss)), kX, kY + 13 * kLine, 18, ORANGE);
    DrawText(TextFormat("seed   %08X  waves %d  marks %d", static_cast<unsigned>(world.seed),
                        static_cast<int>(world.shockwaves.Size()), static_cast<int>(world.spawnMarkers.Size())),
             kX, kY + 14 * kLine, 18, RAYWHITE);
    DrawText(AttackPickLabel(world), kX, kY + 15 * kLine, 18, ORANGE);
    DrawText(ShieldLabel(world.boss), kX, kY + 16 * kLine, 18, ORANGE);
}
#endif

} // namespace

// Every launch starts from a seed the system picks, so no two runs of the game repeat the same fights.
Game::Game() : _session{StartSession(kBossArena, kViewSize, std::random_device{}())} {
    // Vsync caps how often we render. How often the game simulates comes from FixedTimestep.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
}

Game::~Game() {
    CloseWindow();
}

void Game::Run() {
#ifdef __EMSCRIPTEN__
    // A browser tab cannot be held in a loop: the page calls one frame at a time, at its own refresh rate.
    // With the last argument set, this never returns, so `this` stays alive for the life of the page.
    emscripten_set_main_loop_arg([](void* game) { static_cast<Game*>(game)->Frame(); }, this, 0, true);
#else
    while (!WindowShouldClose()) {
        Frame();
    }
#endif
}

void Game::Frame() {
    // Key presses are read once per frame, not per fixed step: a frame with no step would miss them.
    if (ReadRestartPressed()) {
        TryRestart(_session);
    }
    CollectButtonPresses(_pendingPresses);

    // The crosshair replaces the system cursor while playing. On the end screens the cursor is back.
    const bool playing = _session.state == SessionState::Playing;
    if (playing && !IsCursorHidden()) {
        HideCursor();
    } else if (!playing && IsCursorHidden()) {
        ShowCursor();
    }
#ifndef NDEBUG
    if (IsKeyPressed(KEY_F1)) {
        _showDebugOverlay = !_showDebugOverlay;
    }
    // Breaks the shield at once, to reach the exposed window without the right gun.
    if (IsKeyPressed(KEY_F2) && _session.state == SessionState::Playing) {
        BreakBossShield(_session.world);
    }
    // Skips ahead: the transformation in the first phase, the frenzy in the second.
    if (IsKeyPressed(KEY_F3) && _session.state == SessionState::Playing) {
        if (_session.world.boss.phase == BossPhase::One) {
            TransformBoss(_session.world);
        } else {
            FrenzyBoss(_session.world);
        }
    }
    // Skips to the victory screen without emptying both bars. The next step ends the fight.
    if (IsKeyPressed(KEY_F4) && _session.state == SessionState::Playing) {
        DefeatBoss(_session.world);
    }
#endif
    const int steps = _timestep.Advance(GetFrameTime());
    for (int i = 0; i < steps; ++i) {
        Update(static_cast<float>(_timestep.StepSeconds()));
    }
    Draw(static_cast<float>(_timestep.Alpha()));
}

void Game::Update(float dt) {
    // Reading the hardware is the only part of a step that lives in the app. The rules are all in
    // StepSession and StepWorld.
    //
    // The mouse is turned into a world point with the camera as of the last step. The camera drawn on
    // screen is a fraction of a step behind that, which is far below a pixel of aiming error.
    const Camera2D camera = ToRaylibCamera(_session.world.camera, 1.0f);
    const PlayerCommands commands{.move = ReadMoveInput(),
                                  .aimAt = FromRaylib(GetScreenToWorld2D(GetMousePosition(), camera)),
                                  .fireHeld = ReadFireHeld(),
                                  .firePressed = _pendingPresses.fire,
                                  .selectSlot = _pendingPresses.weaponSlot,
                                  .cycleWeapon = _pendingPresses.cycleWeapon,
                                  .dashPressed = _pendingPresses.dash,
                                  .healPressed = _pendingPresses.heal};
    // Presses are events: only the first step of a frame uses them, so a frame that runs two steps does
    // not fire twice for one click.
    _pendingPresses = {};
    StepSession(_session, commands, dt);
#ifndef NDEBUG
    _lastMoveInput = commands.move;
#endif
}

void Game::Draw(float alpha) const {
    BeginDrawing();
    ClearBackground(kBackground);

    const World& world = _session.world;
    const bool playing = _session.state == SessionState::Playing;
    // A frozen world has no next step to blend toward. Interpolating it would make everything shake
    // between its last two positions as alpha changes from frame to frame, so it is drawn where it stopped.
    if (!playing) {
        alpha = 1.0f;
    }

    const Player& player = world.player;
    const Vec2 playerPosition = Lerp(player.motion.previousPosition, player.motion.position, alpha);

    BeginMode2D(ToRaylibCamera(world.camera, alpha));
    DrawArena(world.arena);
    // On the floor, under every body.
    DrawChargeTelegraph(world.boss, world.arena);
    DrawHairRingTelegraph(world.boss);
    DrawLeapTelegraph(world.boss);
    DrawVolleyTelegraph(world.boss);
    for (const Shockwave& wave : world.shockwaves) {
        DrawShockwave(wave);
    }
    for (const SpawnMarker& marker : world.spawnMarkers) {
        DrawSpawnMarker(marker);
    }

    // The boss goes under its children, so a dead boss lies on the floor. In the air it goes over
    // everything, the player included.
    const bool bossAirborne = IsAirborne(world.boss);
    if (!bossAirborne) {
        DrawBoss(world.boss, playerPosition, alpha);
    }
    for (const Enemy& enemy : world.enemies) {
        DrawHairball(enemy, playerPosition, world.boss.summon.spitWarningSeconds, alpha);
    }
    for (const Projectile& bullet : world.bullets) {
        DrawBullet(bullet, alpha);
    }
    for (const Projectile& strand : world.hostileProjectiles) {
        DrawHairStrand(strand, alpha);
    }
    // The player is drawn after every body on the floor, so it is never hidden under a crowd.
    DrawPlayer(player, alpha);
    if (bossAirborne) {
        DrawBoss(world.boss, playerPosition, alpha);
    }
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugWorld(player, world.camera, alpha);
    }
#endif
    EndMode2D();

    // Everything below is in screen space and does not move with the camera.
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugHud(world, _lastMoveInput);
    }
#endif

    DrawHealthBar(player.health);
    DrawDashBar(player.dash);
    DrawFlaskIcons(player.flasks);
    DrawWeaponPanel(player.loadout);
    DrawBossBar(world.boss);
    DrawFPS(10, 10);
    DrawText("Horde Carnage v" HORDE_VERSION, kScreenWidth - 230, 10, 20, kSoftText);
    DrawAttempt(_session.attempt);
    if (playing) {
        DrawText(kControlsHint, kHudMargin, kScreenHeight - 30, 18, kSoftText);
        DrawCrosshair(GetMousePosition());
    } else {
        DrawEndScreen(_session);
    }
    EndDrawing();
}

} // namespace horde
