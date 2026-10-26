#include "app/render/PlayerDraw.h"

#include "app/RaylibConvert.h"
#include "app/render/Palette.h"
#include "core/Loadout.h"

namespace horde {

namespace {

constexpr float kPlayerOutline = 3.0f;
// The eye sits toward the facing direction, so the player reads as pointing somewhere.
constexpr float kEyeOffset = 0.55f;
constexpr float kEyeRadius = 0.28f;
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
// A drink shows as a ring filling around the player, just outside the body.
constexpr float kDrinkRingGap = 4.0f;
constexpr float kDrinkRingWidth = 4.0f;

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

} // namespace

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

} // namespace horde
