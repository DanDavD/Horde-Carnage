#include "app/render/Hud.h"

#include <algorithm>
#include <cstddef>
#include <string>

#include <raylib.h>

#include "app/RaylibConvert.h"
#include "app/render/BossDraw.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/Text.h"
#include "app/render/View.h"
#include "core/Loadout.h"
#include "core/TrailingBar.h"

namespace horde {

namespace {

// In the manner of the games it borrows from, the HUD is thin bars in dark frames and a few words, with no
// numbers on the bars: how much is left is read from the length.
constexpr float kHudMargin = 20.0f;
// The player's bars in the top left corner: health, and the dash recharging under it.
constexpr float kHealthBarWidth = 300.0f;
constexpr float kHealthBarHeight = 12.0f;
constexpr float kDashBarGap = 6.0f;
constexpr float kDashBarHeight = 4.0f;
// Flask icons sit to the right of the health bar.
constexpr float kFlaskIconGap = 16.0f;
constexpr float kFlaskIconWidth = 12.0f;
constexpr float kFlaskIconHeight = 18.0f;
constexpr float kFlaskIconSpacing = 6.0f;
// The boss's health bar runs along the bottom of the screen, under its name, above the controls line, with
// the shield's own bar under it.
constexpr float kBossBarWidth = 640.0f;
constexpr float kBossBarHeight = 10.0f;
constexpr float kBossBarY = kScreenHeight - 92.0f;
constexpr float kShieldBarGap = 4.0f;
constexpr float kShieldBarHeight = 5.0f;
// Weapon panel in the bottom right corner, one box per slot.
constexpr float kSlotBoxWidth = 150.0f;
constexpr float kSlotBoxHeight = 48.0f;
constexpr float kSlotBoxGap = 8.0f;

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

// How far right of the end of the boss's bar the damage tally sits.
constexpr float kTallyX = 12.0f;

// A dark trough in a thin frame, filled from the left by `fraction` with a colour a little lighter along
// its top edge, so the bar reads as a bar of glass rather than a flat stripe. A pale trail runs on from the
// fill to `trailFraction`: what the last run of hits took.
void DrawBar(Rectangle box, float fraction, Color fill, float trailFraction = 0.0f) {
    DrawRectangleRec(Rectangle{box.x - 1.0f, box.y - 1.0f, box.width + 2.0f, box.height + 2.0f}, kHudFrame);
    DrawRectangleRec(box, kBarBack);
    const float width = box.width * std::clamp(fraction, 0.0f, 1.0f);
    const float trailWidth = box.width * std::clamp(trailFraction, 0.0f, 1.0f);
    if (trailWidth > width) {
        DrawRectangleRec(Rectangle{box.x + width, box.y, trailWidth - width, box.height}, kBarTrail);
    }
    DrawRectangleGradientV(static_cast<int>(box.x), static_cast<int>(box.y), static_cast<int>(width),
                           static_cast<int>(box.height), Mix(fill, WHITE, 0.2f), Mix(fill, BLACK, 0.2f));
}

// The share of `health` the trail reaches to.
float TrailFraction(const TrailingBar& trail, const Health& health) {
    return trail.trail / static_cast<float>(health.max);
}

void DrawHealthBar(const Health& health, const TrailingBar& trail) {
    const float fraction = static_cast<float>(health.current) / static_cast<float>(health.max);
    // The fill turns pale while the player is invulnerable, so a hit reads on the bar as well as on the body.
    DrawBar(Rectangle{kHudMargin, kHudMargin, kHealthBarWidth, kHealthBarHeight}, fraction,
            IsInvulnerable(health) ? kBarHurt : kHealth, TrailFraction(trail, health));
}

// Fills up as the dash recharges, and lights up when it is ready.
void DrawDashBar(const Dash& dash) {
    const float total = dash.stats.duration + dash.stats.cooldown;
    const float ready = 1.0f - dash.cooldownSeconds / total;
    const float y = kHudMargin + kHealthBarHeight + kDashBarGap;
    DrawBar(Rectangle{kHudMargin, y, kHealthBarWidth, kDashBarHeight}, ready,
            dash.cooldownSeconds > 0.0f ? kSoftText : kHudAccent);
}

// One glass bottle per charge: amber while full, with a glint of light down one side, and dark when spent.
void DrawFlaskIcons(const Flasks& flasks) {
    const float top = kHudMargin - 3.0f;
    for (int i = 0; i < flasks.stats.maxCharges; ++i) {
        const float x = kHudMargin + kHealthBarWidth + kFlaskIconGap +
                        static_cast<float>(i) * (kFlaskIconWidth + kFlaskIconSpacing);
        const Rectangle bottle{x, top + 4.0f, kFlaskIconWidth, kFlaskIconHeight};
        const Rectangle neck{x + 3.0f, top, kFlaskIconWidth - 6.0f, 5.0f};
        const bool full = i < flasks.charges;
        DrawRectangleRec(neck, kHudFrame);
        DrawRectangleRounded(bottle, 0.5f, 6, full ? kFlask : kBarBack);
        if (full) {
            DrawRectangleRec(Rectangle{x + 2.5f, top + 7.0f, 2.0f, kFlaskIconHeight - 7.0f}, Fade(WHITE, 0.45f));
        }
        DrawRectangleRoundedLinesEx(bottle, 0.5f, 6, 1.0f, kHudFrame);
    }
}

// Under the boss's bar: what is left of the shield, in its colour, with its name at the right end above the
// bar. Once it breaks, a white strip drains away with the exposed time, so the player can see how long the
// opening lasts.
void DrawShieldBar(const Shield& shield, float x, const Art& art) {
    float fraction = shield.exposedSecondsLeft / shield.stats.exposedSeconds;
    Color color = kExposed;
    const char* label = "EXPOSED";
    if (IsShieldUp(shield)) {
        fraction = shield.strength / MaxStrength(shield.stats, shield.kind);
        color = ShieldColor(shield.kind);
        label = shield.kind == ShieldKind::Shell ? "SHELL" : "BARRIER";
    }
    const float y = kBossBarY + kBossBarHeight + kShieldBarGap;
    DrawRectangleRec(Rectangle{x, y, kBossBarWidth, kShieldBarHeight}, kBarBack);
    DrawRectangleRec(Rectangle{x, y, kBossBarWidth * std::clamp(fraction, 0.0f, 1.0f), kShieldBarHeight}, color);
    DrawGameTextRight(art.uiFont, label, x + kBossBarWidth, kBossBarY - 24.0f, 16.0f, color);
}

// The damage of the current run of hits, just right of the bar, as Dark Souls shows it. It stays while the
// hits keep coming and fades out as the trail drains.
void DrawDamageTally(const TrailingBar& trail, const Health& health, float x, const Art& art) {
    if (trail.tally <= 0 || !IsTrailing(trail, health.current)) {
        return;
    }
    const float left = trail.trail - static_cast<float>(health.current);
    const float draining = std::clamp(left / static_cast<float>(trail.tally), 0.0f, 1.0f);
    const float shown = trail.holdSecondsLeft > 0.0f ? 1.0f : draining;
    DrawGameText(art.uiFont, TextFormat("%d", trail.tally), Vec2{x + kBossBarWidth + kTallyX, kBossBarY - 4.0f},
                 16.0f, Fade(kText, shown));
}

// Wide and thin along the bottom of the screen, with the name above it, so it reads as the one fight that
// matters and stays out of the way of the arena.
void DrawBossBar(const Boss& boss, const TrailingBar& trail, const Art& art) {
    constexpr float kX = (kScreenWidth - kBossBarWidth) / 2.0f;
    const bool transforming = IsTransforming(boss);
    float fraction = static_cast<float>(boss.health.current) / static_cast<float>(boss.health.max);
    // While it transforms, the bar fills back up over the transformation, and the old trail has no meaning.
    if (transforming) {
        fraction = 1.0f - boss.transformSecondsLeft / boss.phaseTwo.transformSeconds;
    }
    DrawGameText(art.titleFont, kBossName, Vec2{kX, kBossBarY - 30.0f}, 24.0f, kText);
    DrawBar(Rectangle{kX, kBossBarY, kBossBarWidth, kBossBarHeight}, fraction, kBossBar,
            transforming ? 0.0f : TrailFraction(trail, boss.health));
    if (!transforming) {
        DrawDamageTally(trail, boss.health, kX, art);
    }
    if (!IsDead(boss.health) && !transforming) {
        DrawShieldBar(boss.shield, kX, art);
    }
}

// One box per slot. The gun in hand is framed in bone, so the player can tell at a glance which one they
// hold.
void DrawWeaponPanel(const Loadout& loadout, const Art& art) {
    const float top = kScreenHeight - kSlotBoxHeight - kHudMargin;
    for (int slot = 0; slot < kWeaponSlots; ++slot) {
        const Weapon& weapon = loadout.slots[static_cast<std::size_t>(slot)];
        const bool inHand = slot == loadout.equipped;
        const int slotsToTheRight = kWeaponSlots - slot;
        const float left = kScreenWidth - kHudMargin -
                           static_cast<float>(slotsToTheRight) * (kSlotBoxWidth + kSlotBoxGap) + kSlotBoxGap;
        const Rectangle box{left, top, kSlotBoxWidth, kSlotBoxHeight};

        DrawRectangleRec(box, Fade(BLACK, inHand ? 0.6f : 0.4f));
        DrawRectangleLinesEx(box, inHand ? 1.5f : 1.0f, inHand ? kHudAccent : kHudFrame);
        const Color text = inHand ? kText : kSoftText;
        const std::string name = TextToUpper(std::string(weapon.stats.name).c_str());
        DrawGameText(art.uiFont, TextFormat("%d  %s", slot + 1, name.c_str()), Vec2{left + 10.0f, top + 7.0f}, 18.0f,
                     text);
        DrawGameText(art.uiFont, FireModeLabel(weapon.stats.mode), Vec2{left + 10.0f, top + 28.0f}, 12.0f,
                     Fade(text, 0.8f));
    }
}

// Drawn in place of the system cursor while playing, so the aim point is easy to find in a crowd. A dark
// outline under the pale lines keeps it visible over the torchlit floor and the dark alike.
void DrawCrosshair(Vector2 at) {
    constexpr float kRadius = 10.0f;
    constexpr float kTick = 6.0f;
    for (const bool outline : {true, false}) {
        const Color color = outline ? Fade(BLACK, 0.6f) : kCrosshair;
        const float width = outline ? 4.0f : 2.0f;
        DrawRing(at, kRadius - width * 0.5f, kRadius + width * 0.5f, 0.0f, 360.0f, 32, color);
        DrawLineEx({at.x - kRadius - kTick, at.y}, {at.x - kRadius + 3.0f, at.y}, width, color);
        DrawLineEx({at.x + kRadius - 3.0f, at.y}, {at.x + kRadius + kTick, at.y}, width, color);
        DrawLineEx({at.x, at.y - kRadius - kTick}, {at.x, at.y - kRadius + 3.0f}, width, color);
        DrawLineEx({at.x, at.y + kRadius - 3.0f}, {at.x, at.y + kRadius + kTick}, width, color);
    }
    DrawCircleV(at, 1.5f, kCrosshair);
}

} // namespace

void DrawHud(const World& world, const HealthTrails& trails, int attempt, bool playing, Vec2 aim, const Art& art) {
    const Player& player = world.player;
    DrawHealthBar(player.health, trails.player);
    DrawDashBar(player.dash);
    DrawFlaskIcons(player.flasks);
    DrawWeaponPanel(player.loadout, art);
    DrawBossBar(world.boss, trails.boss, art);

    // The version and the attempt in the top right corner, so the count of tries is always in sight.
    constexpr float kRight = kScreenWidth - kHudMargin;
    DrawGameTextRight(art.uiFont, "Horde Carnage v" HORDE_VERSION, kRight, kHudMargin - 4.0f, 14.0f, kSoftText);
    DrawGameTextRight(art.titleFont, TextFormat("Attempt %d", attempt), kRight, kHudMargin + 14.0f, 22.0f, kText);
    DrawGameText(art.uiFont, TextFormat("%d FPS", GetFPS()), Vec2{kHudMargin, kScreenHeight - 56.0f}, 12.0f,
                 Fade(kSoftText, 0.7f));
    if (playing) {
        DrawGameText(art.uiFont, kControlsHint, Vec2{kHudMargin, kScreenHeight - 34.0f}, 14.0f, kSoftText);
        DrawCrosshair(ToRaylib(aim));
    }
}

} // namespace horde
