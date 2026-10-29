#include "app/render/PlayerDraw.h"

#include <algorithm>
#include <cmath>

#include "app/RaylibConvert.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/Shapes.h"
#include "core/Loadout.h"

namespace horde {

namespace {

// The hunter, seen from above. Sizes are fractions of the body radius, so the drawing follows
// `Player::radius`, and the body circle the rules use is the round part of the cape. Outlines are in world
// units.
constexpr float kOutline = 2.0f;
// The cape trails this far behind the body when standing, and further at full speed, swaying a little
// from side to side. Its hem is torn into teeth of uneven depth.
constexpr float kCapeTail = 0.6f;
constexpr float kCapeFlare = 0.5f;
constexpr float kCapeSway = 0.18f; // Radians.
constexpr float kCapeSwaySpeed = 5.0f;
constexpr float kCapeTeeth = 18.0f;
constexpr float kCapeTear = 0.3f;
// The long coat across the shoulders, wider than deep, so the body reads as turned toward the aim.
constexpr float kCoatWidth = 0.92f;
constexpr float kCoatDepth = 0.62f;
// A bandolier of brass cartridges across the chest, from the left shoulder to the right hip.
constexpr float kBandolierWidth = 4.0f;
constexpr int kCartridges = 5;
constexpr float kCartridgeRadius = 1.9f;
// The arms reach from the shoulders to the gun. Both hands are always on it.
constexpr float kShoulderSpread = 0.62f;
constexpr float kShoulderForward = 0.1f;
constexpr float kArmWidth = 0.3f;
constexpr float kGloveRadius = 0.19f;
// The pistol is held out at arm's length in both hands, the arms a triangle in front of the chest, the
// hands cupped together on the grip.
constexpr float kPistolHold = 1.05f;
constexpr float kPistolHandSpread = 2.2f;
constexpr float kPistolWidth = 6.0f;
constexpr float kPistolDrumRadius = 4.2f;
// The SMG is held like a rifle: the stock against the chest, the right hand on the grip close to the
// body, the left on the foregrip further out, and the magazine sticking out to the left.
constexpr float kSmgStock = 0.3f;
constexpr float kSmgReceiver = 0.75f;
constexpr float kSmgGrip = 0.9f;
constexpr float kSmgForegrip = 1.6f;
constexpr float kSmgMagazine = 1.25f;
constexpr float kSmgMagazineLength = 9.0f;
constexpr float kSmgWidth = 9.0f;
// The gun not in hand is carried where it can be seen: the SMG slung across the back, the pistol in a
// holster on the right hip. Pressing Q swaps them over.
constexpr float kSlungWidth = 7.0f;
constexpr float kHolsterSide = 0.82f;
// The tricorn: three points, one over the face, so the hat turns with the aim; a worn trim along its
// brim, and the round crown in the middle.
constexpr float kHatRadius = 0.5f;
constexpr float kHatBase = 0.78f;
constexpr float kHatPoint = 0.4f;
constexpr float kHatFrontPoint = 0.5f;
constexpr float kHatPointSharpness = 8.0f;
constexpr float kHatForward = 0.05f;
constexpr float kHatCrownRadius = 0.3f;
// While invulnerable the player blinks: faded for one period, solid for the next.
constexpr float kBlinkPeriod = 0.1f;
constexpr float kBlinkFade = 0.35f;
// A dead hunter's colours sink this far toward grey.
constexpr float kDeadGrey = 0.65f;
// The dash leaves a fading trail of ghosts between where it started and the body, so a move that takes
// seven frames still reads as a jump.
constexpr int kDashGhosts = 4;
// A drink shows as a ring filling around the player, just outside the cape.
constexpr float kDrinkRingGap = 4.0f;
constexpr float kDrinkRingWidth = 4.0f;
constexpr float kMuzzleFlashRadius = 8.0f;

// A straight piece, from `from` to `to`, with a dark outline all round it: an arm, a gun, a strap.
void DrawPiece(Vec2 from, Vec2 to, float width, Color fill, Color edge) {
    const Vec2 along = Normalized(to - from) * (kOutline * 0.5f);
    DrawLineEx(ToRaylib(from - along), ToRaylib(to + along), width + kOutline, edge);
    DrawLineEx(ToRaylib(from), ToRaylib(to), width, fill);
}

// The hem's torn teeth, from 1 at the root of a tooth down to its tip. `fromTrail` is the angle from the
// way the cape trails, so the teeth turn with the cape.
float Tatter(float fromTrail) {
    const float turns = (fromTrail + kPi) / (2.0f * kPi) * kCapeTeeth;
    const float tooth = std::floor(turns);
    const float depth = 0.5f + 0.5f * Hash(tooth);
    return 1.0f - kCapeTear * depth * (turns - tooth);
}

// The way the cape trails: behind the back when standing, behind the movement when running, turning
// between the two the short way round.
float TrailAngle(const Player& player, Vec2 facing, float seconds) {
    const Vec2 velocity = player.motion.velocity;
    const float speed = std::min(Length(velocity) / player.stats.maxSpeed, 1.0f);
    const float back = Angle(facing * -1.0f);
    float behind = back;
    if (speed > 0.01f) {
        const float turn = std::remainder(Angle(velocity * -1.0f) - back, 2.0f * kPi);
        behind = back + turn * speed;
    }
    return behind + std::sin(seconds * kCapeSwaySpeed) * kCapeSway * (0.3f + speed);
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
        DrawCircleV(ToRaylib(ghost), size, Fade(kCoat, 0.5f * strength * along));
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

} // namespace

void DrawPlayer(const Player& player, float seconds, float alpha, std::optional<Vec2> aimDirection) {
    // Drawn between the last two fixed steps, so movement is smooth above 60 Hz.
    const Vec2 position = Lerp(player.motion.previousPosition, player.motion.position, alpha);
    const Health& health = player.health;
    const bool dead = IsDead(health);
    DrawDashTrail(player, position);
    DrawDrinkRing(player, position);

    // The blink tells the player they were hit and that the next touch will not hurt yet.
    const bool fadedHalf = static_cast<int>(health.invulnerableSeconds / kBlinkPeriod) % 2 == 1;
    const float opacity = IsInvulnerable(health) && fadedHalf ? kBlinkFade : 1.0f;
    const auto paint = [dead, opacity](Color color) {
        return Fade(dead ? Mix(color, kPlayerDeadFill, kDeadGrey) : color, opacity);
    };
    const Color edge = paint(kCapeEdge);

    const float r = player.radius;
    const Vec2 facing =
        (aimDirection && LengthSquared(*aimDirection) > 1e-6f) ? Normalized(*aimDirection) : player.facing;
    const Vec2 side{-facing.y, facing.x}; // The hunter's right.
    const auto at = [&](float forward, float right) { return position + facing * (r * forward) + side * (r * right); };

    // The cape: the round body, with a torn tail that trails behind it. A dead hunter's cape lies still.
    const float speed = std::min(Length(player.motion.velocity) / player.stats.maxSpeed, 1.0f);
    const float trail = TrailAngle(player, facing, dead ? 0.0f : seconds);
    const float tail = r * (kCapeTail + kCapeFlare * speed);
    const auto cape = [&](float extra) {
        return [&, extra](float angle) {
            const float fromTrail = std::remainder(angle - trail, 2.0f * kPi);
            return r + extra + tail * Bulge(angle, trail, 3.0f) * Tatter(fromTrail);
        };
    };
    DrawRadialShape<96>(position, cape(kOutline), edge);
    DrawRadialShape<96>(position, cape(0.0f), paint(kCape));

    // The coat over the shoulders, and the gun not in hand on its strap or in its holster.
    DrawOrientedEllipse(position, side, r * kCoatWidth + kOutline, r * kCoatDepth + kOutline, edge);
    DrawOrientedEllipse(position, side, r * kCoatWidth, r * kCoatDepth, paint(kCoat));
    const Weapon& weapon = Equipped(player.loadout);
    const bool smgInHand = weapon.stats.mode == FireMode::Auto;
    if (smgInHand) {
        DrawPiece(at(-0.15f, kHolsterSide), at(-0.6f, kHolsterSide), kPistolWidth, paint(kLeather), edge);
        DrawPiece(at(-0.15f, kHolsterSide), at(0.05f, kHolsterSide), kPistolWidth * 0.7f, paint(kGunDark), edge);
    } else {
        DrawPiece(at(-0.2f, -0.75f), at(-0.5f, 0.05f), kSlungWidth, paint(kGunWood), edge);
        DrawPiece(at(-0.5f, 0.05f), at(-0.8f, 0.8f), kSlungWidth, paint(kGunDark), edge);
    }

    // The bandolier, with its cartridges catching the light.
    const Vec2 strapFrom = at(0.35f, -0.7f);
    const Vec2 strapTo = at(-0.45f, 0.62f);
    DrawPiece(strapFrom, strapTo, kBandolierWidth, paint(kLeather), edge);
    for (int i = 0; i < kCartridges; ++i) {
        const float along = 0.15f + 0.7f * static_cast<float>(i) / static_cast<float>(kCartridges - 1);
        DrawCircleV(ToRaylib(Lerp(strapFrom, strapTo, along)), kCartridgeRadius, paint(kBrass));
    }

    // The gun in both hands. The muzzle is where the bullets leave from, the same `MuzzlePoint` the rules
    // use, so the shots come out of the barrel.
    const Vec2 muzzle = MuzzlePoint(position, facing, r, weapon.stats);
    const Vec2 rightShoulder = at(kShoulderForward, kShoulderSpread);
    const Vec2 leftShoulder = at(kShoulderForward, -kShoulderSpread);
    Vec2 rightHand;
    Vec2 leftHand;
    if (smgInHand) {
        rightHand = at(kSmgGrip, 0.08f);
        leftHand = at(kSmgForegrip, -0.05f);
    } else {
        const Vec2 grip = at(kPistolHold, 0.0f);
        rightHand = grip + side * kPistolHandSpread;
        leftHand = grip - side * kPistolHandSpread;
    }
    DrawPiece(rightShoulder, rightHand, r * kArmWidth, paint(kCoat), edge);
    DrawPiece(leftShoulder, leftHand, r * kArmWidth, paint(kCoat), edge);
    if (smgInHand) {
        DrawPiece(at(kSmgMagazine, 0.0f), at(kSmgMagazine, 0.0f) - side * kSmgMagazineLength, 5.0f,
                  paint(kGunDark), edge);
        DrawPiece(at(kSmgStock, 0.0f), at(kSmgReceiver, 0.0f), kSmgWidth * 0.8f, paint(kGunWood), edge);
        DrawPiece(at(kSmgReceiver, 0.0f), muzzle, kSmgWidth, paint(kGunDark), edge);
        DrawLineEx(ToRaylib(at(kSmgReceiver, 0.0f) + side), ToRaylib(muzzle + side - facing * 2.0f), 2.5f,
                   paint(kGun));
    } else {
        const Vec2 grip = at(kPistolHold, 0.0f);
        DrawPiece(grip - facing * 3.0f, muzzle, kPistolWidth, paint(kGunDark), edge);
        DrawLineEx(ToRaylib(grip + facing * 2.0f + side), ToRaylib(muzzle + side - facing), 2.0f, paint(kGun));
        const Vec2 drum = grip + facing * 5.5f;
        DrawCircleV(ToRaylib(drum), kPistolDrumRadius + kOutline * 0.5f, edge);
        DrawCircleV(ToRaylib(drum), kPistolDrumRadius, paint(kGunDark));
        DrawCircleV(ToRaylib(drum), kPistolDrumRadius * 0.4f, paint(kGun));
    }
    for (const Vec2 hand : {rightHand, leftHand}) {
        DrawCircleV(ToRaylib(hand), r * kGloveRadius + kOutline * 0.5f, edge);
        DrawCircleV(ToRaylib(hand), r * kGloveRadius, paint(kGlove));
    }

    // The tricorn on top of it all, its front point over the face.
    const Vec2 head = at(kHatForward, 0.0f);
    const float front = Angle(facing);
    const auto hat = [&](float extra) {
        return [&, extra](float angle) {
            float points = kHatFrontPoint * Bulge(angle, front, kHatPointSharpness);
            points += kHatPoint * Bulge(angle, front + 2.0f * kPi / 3.0f, kHatPointSharpness);
            points += kHatPoint * Bulge(angle, front - 2.0f * kPi / 3.0f, kHatPointSharpness);
            return r * kHatRadius * (kHatBase + points) + extra;
        };
    };
    DrawRadialShape<72>(head, hat(kOutline), edge);
    DrawRadialShape<72>(head, hat(0.0f), paint(kHatTrim));
    DrawRadialShape<72>(head, hat(-kOutline * 1.2f), paint(kHat));
    DrawCircleV(ToRaylib(head), r * kHatCrownRadius + kOutline * 0.5f, edge);
    DrawCircleV(ToRaylib(head), r * kHatCrownRadius, paint(kHatCrown));

    if (weapon.muzzleFlashSeconds > 0.0f) {
        DrawCircleV(ToRaylib(muzzle), kMuzzleFlashRadius, kMuzzleFlash);
        DrawCircleV(ToRaylib(muzzle), kMuzzleFlashRadius * 0.45f, kHitFlash);
    }
}

} // namespace horde
