#include "app/render/HairBody.h"

#include <cmath>
#include <numbers>

#include "app/RaylibConvert.h"
#include "app/render/Palette.h"

namespace horde {

namespace {

constexpr float kHairballEyeOffset = 0.45f;
constexpr float kHairballEyeSpread = 0.35f;
constexpr float kHairballEyeRadius = 0.22f;

constexpr HairStyle kHairballStyle{kHairballFill, kHairballEdge, kHairballEye, 12, 3.0f, 3.0f, 1.0f};
constexpr HairStyle kSpittingHairballStyle{kHairballFill, kHairballEdge, kBlood, 12, 3.0f, 3.0f, 1.0f};
constexpr float kHairballSpitSwell = 0.2f;

} // namespace

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

} // namespace horde
