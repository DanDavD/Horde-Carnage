#include "app/render/Hud.h"

#include <algorithm>
#include <cstddef>
#include <string>

#include <raylib.h>

#include "app/RaylibConvert.h"
#include "app/render/BossDraw.h"
#include "app/render/Palette.h"
#include "app/render/View.h"
#include "core/Loadout.h"

namespace horde {

namespace {

// The boss's health bar runs along the bottom of the screen, under its name, above the controls line.
constexpr int kBossBarWidth = 640;
constexpr int kBossBarHeight = 12;
constexpr int kBossBarY = kScreenHeight - 96;
// The shield's own bar runs under the boss's.
constexpr int kShieldBarHeight = 6;
// The dash bar sits under the health bar.
constexpr int kDashBarY = 64;
constexpr int kDashBarHeight = 6;
// Flask icons sit to the right of the health bar.
constexpr int kFlaskIconX = 282;
constexpr int kFlaskIconWidth = 12;
constexpr int kFlaskIconHeight = 18;
constexpr int kFlaskIconGap = 6;
// Weapon panel in the bottom right corner, one box per slot.
constexpr int kSlotBoxWidth = 150;
constexpr int kSlotBoxHeight = 48;
constexpr int kSlotBoxGap = 8;
constexpr int kHudMargin = 12;

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

// Under the version, in the top right corner, so the count of tries is always in sight.
void DrawAttempt(int attempt) {
    const char* text = TextFormat("Attempt %d", attempt);
    DrawText(text, kScreenWidth - kHudMargin - MeasureText(text, 20), 36, 20, kSoftText);
}

} // namespace

void DrawHud(const World& world, int attempt, bool playing, Vec2 aim) {
    const Player& player = world.player;
    DrawHealthBar(player.health);
    DrawDashBar(player.dash);
    DrawFlaskIcons(player.flasks);
    DrawWeaponPanel(player.loadout);
    DrawBossBar(world.boss);
    DrawFPS(10, 10);
    DrawText("Horde Carnage v" HORDE_VERSION, kScreenWidth - 230, 10, 20, kSoftText);
    DrawAttempt(attempt);
    if (playing) {
        DrawText(kControlsHint, kHudMargin, kScreenHeight - 30, 18, kSoftText);
        DrawCrosshair(ToRaylib(aim));
    }
}

} // namespace horde
