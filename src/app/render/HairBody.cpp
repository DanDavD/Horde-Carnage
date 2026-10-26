#include "app/render/HairBody.h"

#include <algorithm>
#include <cmath>

#include "app/RaylibConvert.h"
#include "app/render/Palette.h"
#include "app/render/Shapes.h"

namespace horde {

namespace {

// Points along the outline: enough for the most locks any body has to come to a point each.
constexpr int kOutlinePoints = 336;
// The outline is this share of the radius, but never thinner than this many units.
constexpr float kOutlineShare = 0.05f;
constexpr float kMinOutline = 2.0f;
// The body breathes: it swells and shrinks a little, and every lock grows and shrinks out of step with the
// others. A dead body's locks lie flatter.
constexpr float kBreathSpeed = 2.2f;
constexpr float kBreathDepth = 0.02f;
constexpr float kLockBreath = 0.2f;
constexpr float kDeadLocks = 0.55f;
// An inner layer of shorter locks, between the outer ones, gives the hair depth.
constexpr float kInnerLayer = 0.8f;
constexpr float kInnerLength = 0.6f;
// The body is not round: a few big, slow lumps, as if the hair were matted into clumps.
constexpr float kLumpDepth = 0.07f;
// The sheen: a patch of lighter hair shifted toward the light.
constexpr float kSheenSize = 0.55f;
constexpr float kSheenShift = 0.3f;
constexpr float kSheenStrength = 0.35f;
// Short strands drawn into the hair, each sweeping round the body a little, the way matted hair lies.
constexpr float kStrandsPerLock = 0.6f;
constexpr float kStrandWidth = 0.035f;
// The eyes sit toward where the body looks: slits tilted down toward the middle, glowing on the hair around
// them, with a narrow pupil across.
constexpr float kEyeForward = 0.42f;
constexpr float kEyeSpread = 0.3f;
constexpr float kSlitLength = 0.17f;
constexpr float kSlitHeight = 0.065f;
constexpr float kSlitTilt = 0.35f; // Radians.
constexpr float kEyeGlowSize = 0.55f;
constexpr float kEyeGlowStrength = 0.4f;
// The mouth, in front of the eyes: a dark slit that opens into a jaw full of teeth.
constexpr float kMouthForward = 0.7f;
constexpr float kMouthWidth = 0.36f;
constexpr float kMouthShut = 0.05f;
constexpr float kMouthOpen = 0.26f;
constexpr int kTeethPerJaw = 7;
constexpr float kToothLength = 0.08f;
// Hairballs: a child of the boss, lighter and smaller, with its mother's eyes, swelling and bristling before
// it spits.
constexpr HairStyle kHairballStyle{kHairballFill, kHairballEdge, kHairballSheen, kHairballEye, 13, 0.38f, 0.25f, 1.0f,
                                   1.3f};
constexpr HairStyle kSpittingHairballStyle{kHairballFill, kHairballEdge, kHairballSheen, kBlood, 13, 0.46f, 0.8f, 1.0f,
                                           1.3f};
constexpr float kHairballSpitSwell = 0.2f;

// How far a lock reaches past the body at `angle`, as a share of the radius. `layer` 0 is the outer
// locks, 1 the inner ones between them, and 2 the sheen.
float LockReach(const HairStyle& style, float angle, int layer, float seconds) {
    const float locks = static_cast<float>(style.locks);
    float u = angle / (2.0f * kPi) * locks + (layer == 1 ? 0.5f : 0.0f);
    u -= std::floor(u / locks) * locks;
    const float lock = std::floor(u);
    const float fromMiddle = std::abs(u - lock - 0.5f) * 2.0f;
    const float h = Hash(lock + 17.0f * static_cast<float>(layer));
    const float breath = 1.0f + kLockBreath * std::sin(seconds * kBreathSpeed * (0.7f + 0.6f * h) + 2.0f * kPi * h);
    const float length = style.lockLength * (0.5f + 0.5f * h) * breath;
    // Soft locks are rounded lumps; stiff ones come to a point.
    const float soft = 0.5f + 0.5f * std::cos(kPi * fromMiddle);
    const float stiff = std::pow(1.0f - fromMiddle, 3.5f);
    return length * (soft + (stiff - soft) * style.bristle);
}

// The slow lumps of the body at `angle`, as a share of the radius. They shift a little as it breathes.
float Lumps(float angle, float seconds) {
    const float drift = 0.4f * std::sin(seconds * 0.7f);
    return kLumpDepth * (0.6f * std::sin(3.0f * angle + 1.3f + drift) + 0.4f * std::sin(5.0f * angle + 4.1f - drift));
}

} // namespace

Color Mix(Color from, Color to, float amount) {
    const auto channel = [amount](unsigned char a, unsigned char b) {
        const float mixed = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * amount;
        return static_cast<unsigned char>(mixed);
    };
    return Color{channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), channel(from.a, to.a)};
}

void DrawHairBody(Vec2 position, float radius, const HairStyle& style, const HairPose& pose, const Art& art) {
    // A hit turns the body toward white for a moment, so every bullet that lands is visible.
    const float flash = pose.flashing ? style.flash : 0.0f;
    const Color fill = Mix(style.fill, kHitFlash, flash);
    const Color edge = Mix(style.edge, kHitFlash, flash * 0.5f);
    const Color sheen = Mix(style.sheen, kHitFlash, flash);
    const float seconds = pose.dead ? 0.0f : pose.seconds;
    const float locks = pose.dead ? kDeadLocks : 1.0f;
    const float r = radius * (1.0f + (pose.dead ? 0.0f : kBreathDepth * std::sin(seconds * kBreathSpeed)));
    const float outline = std::max(kMinOutline, radius * kOutlineShare);

    // Squash and stretch: the body keeps its area, longer one way and thinner the other.
    const Vec2 along = pose.stretchAlong;
    const Vec2 across{-along.y, along.x};
    const auto warp = [&](Vec2 offset) {
        return along * (Dot(offset, along) * pose.stretch) + across * (Dot(offset, across) / pose.stretch);
    };
    const auto at = [&](Vec2 offset) { return position + warp(offset); };

    const auto layer = [&](int which, float base, float scale, float extra) {
        return [&, which, base, scale, extra](float angle) {
            const float lock = LockReach(style, angle, which, seconds) * locks;
            return r * (base + (Lumps(angle, seconds) + lock) * scale) + extra;
        };
    };
    DrawRadialShape<kOutlinePoints>(position, layer(0, 1.0f, 1.0f, outline), warp, edge);
    DrawRadialShape<kOutlinePoints>(position, layer(0, 1.0f, 1.0f, 0.0f), warp, fill);
    DrawRadialShape<kOutlinePoints>(position, layer(1, kInnerLayer, kInnerLength, outline * 0.6f), warp,
                                    Mix(fill, edge, 0.45f));
    DrawRadialShape<kOutlinePoints>(position, layer(1, kInnerLayer, kInnerLength, 0.0f), warp,
                                    Mix(fill, sheen, 0.15f));

    // The side toward the torch catches the light.
    const Vec2 toLight = LengthSquared(pose.light - position) > 1.0f ? Normalized(pose.light - position)
                                                                       : Vec2{0.0f, -1.0f};
    DrawRadialShape<kOutlinePoints>(at(toLight * (r * kSheenShift)), layer(2, kSheenSize, 0.5f, 0.0f), warp,
                                    Fade(sheen, kSheenStrength));

    const int strands = static_cast<int>(static_cast<float>(style.locks) * kStrandsPerLock);
    const float strandWidth = std::max(1.0f, r * kStrandWidth);
    for (int i = 0; i < strands; ++i) {
        const float n = static_cast<float>(i);
        const float angle = 2.0f * kPi * n / static_cast<float>(strands) + 0.5f * Hash(3.1f * n);
        const float start = 0.25f + 0.4f * Hash(7.3f * n);
        const float length = 0.2f + 0.2f * Hash(9.1f * n);
        // Every other strand sweeps the other way, so they cross like tangled hair.
        const float sweep = (0.5f + 0.5f * Hash(5.7f * n)) * (i % 2 == 0 ? 1.0f : -1.0f);
        const Vec2 from = at(Rotated(Vec2{r * start, 0.0f}, angle));
        const Vec2 middle = at(Rotated(Vec2{r * (start + length * 0.5f), 0.0f}, angle + sweep * 0.35f));
        const Vec2 to = at(Rotated(Vec2{r * (start + length), 0.0f}, angle + sweep * 0.6f));
        DrawLineEx(ToRaylib(from), ToRaylib(middle), strandWidth, Fade(edge, 0.5f));
        DrawLineEx(ToRaylib(middle), ToRaylib(to), strandWidth, Fade(edge, 0.5f));
    }

    // The face turns to where the body looks.
    const Vec2 look = LengthSquared(pose.lookAt - position) > 1.0f ? Normalized(pose.lookAt - position)
                                                                     : Vec2{0.0f, 1.0f};
    const Vec2 side{-look.y, look.x};
    if (pose.mouth >= 0.0f && !pose.dead) {
        const Vec2 mouth = at(look * (r * kMouthForward));
        const float width = r * kMouthWidth;
        const float height = r * (kMouthShut + (kMouthOpen - kMouthShut) * std::clamp(pose.mouth, 0.0f, 1.0f));
        DrawOrientedEllipse(mouth, side, width + outline * 0.5f, height + outline * 0.5f, edge);
        DrawOrientedEllipse(mouth, side, width, height, kMouth);
        // Teeth along both jaws, pointing in, longer than the gap is wide when it is shut.
        const float tooth = std::min(r * kToothLength, height * 1.2f);
        for (int i = 0; i < kTeethPerJaw; ++i) {
            const float x = -1.0f + (static_cast<float>(i) + 0.5f) * 2.0f / kTeethPerJaw;
            const float jaw = height * std::sqrt(std::max(1.0f - x * x, 0.0f));
            const float halfWidth = width / kTeethPerJaw * 0.7f;
            for (const float lip : {-1.0f, 1.0f}) {
                const Vec2 root = mouth + side * (x * width) + look * (lip * jaw);
                DrawTriangleAnyWinding(root - side * halfWidth, root + side * halfWidth, root - look * (lip * tooth),
                                       kTooth);
            }
        }
    }
    for (const float which : {-1.0f, 1.0f}) {
        // Bigger eyes sit a little further apart, so they never touch.
        const float spread = kEyeSpread * (0.5f + 0.5f * style.eyeSize);
        const Vec2 eye = at(look * (r * kEyeForward) + side * (which * r * spread));
        if (pose.dead) {
            const float length = r * kSlitLength * style.eyeSize;
            DrawLineEx(ToRaylib(eye - side * length), ToRaylib(eye + side * length), outline, edge);
            continue;
        }
        BeginBlendMode(BLEND_ADDITIVE);
        DrawGlow(art, eye, r * kEyeGlowSize, Fade(style.eye, kEyeGlowStrength));
        EndBlendMode();
        // Tilted so the inner corners point down toward the mouth, which reads as a scowl.
        const Vec2 axis = Rotated(side, which * kSlitTilt);
        const Vec2 pupil{-axis.y, axis.x};
        const float length = r * kSlitLength * style.eyeSize;
        const float height = r * kSlitHeight * style.eyeSize;
        DrawOrientedEllipse(eye, axis, length + outline * 0.5f, height + outline * 0.5f, edge);
        DrawOrientedEllipse(eye, axis, length, height, style.eye);
        DrawOrientedEllipse(eye, pupil, height * 0.85f, length * 0.16f, kPupil);
    }
}

// A hairball about to spit swells, bristles and reddens its eyes, so the player can see which one is next.
void DrawHairball(const Enemy& enemy, Vec2 playerPosition, float spitWarningSeconds, float seconds, float alpha,
                  const Art& art) {
    const Vec2 position = Lerp(enemy.motion.previousPosition, enemy.motion.position, alpha);
    const bool aboutToSpit = enemy.spits && enemy.secondsUntilSpit <= spitWarningSeconds;
    const float radius = enemy.radius * (aboutToSpit ? 1.0f + kHairballSpitSwell : 1.0f);
    const HairPose pose{.lookAt = playerPosition,
                        .light = playerPosition,
                        .seconds = seconds,
                        .flashing = enemy.hitFlashSeconds > 0.0f};
    DrawHairBody(position, radius, aboutToSpit ? kSpittingHairballStyle : kHairballStyle, pose, art);
}

} // namespace horde
