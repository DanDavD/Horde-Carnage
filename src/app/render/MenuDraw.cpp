#include "app/render/MenuDraw.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "app/RaylibConvert.h"
#include "app/render/Palette.h"
#include "app/render/Text.h"
#include "app/render/View.h"

namespace horde {

namespace {

// The menus, in the manner of Dark Souls: a few words in capitals, spaced wide, on the dark. The selected
// one is lit and framed by a line and a diamond on each side, in the manner of Hollow Knight.
constexpr float kItemTracking = 0.22f;
constexpr float kMarkGap = 14.0f;    // From the end of the word to the diamond.
constexpr float kMarkLine = 28.0f;   // The line beyond the diamond.
constexpr float kMarkDiamond = 4.0f; // The diamond's half width.
constexpr float kMarkSlideSeconds = 0.12f;
// The box the mouse can hit around an item, beyond its letters.
constexpr float kHitPadX = 18.0f;
constexpr float kHitPadY = 10.0f;

constexpr float kMiddle = kScreenWidth / 2.0f;

// The title and its menu are one block in the middle of the screen.
constexpr float kTitleSize = 76.0f;
constexpr float kTitleY = kScreenHeight / 2.0f - 108.0f;
constexpr float kTitleRuleY = kTitleY + kTitleSize + 22.0f;
constexpr float kTitleRuleHalf = 90.0f;
constexpr float kTitleMenuY = kTitleRuleY + 62.0f;

constexpr float kPauseSize = 40.0f;
constexpr float kPauseY = kScreenHeight / 2.0f - 100.0f;
constexpr float kPauseMenuY = kPauseY + kPauseSize + 70.0f;
constexpr float kMenuSpacing = 46.0f;
constexpr float kPauseFadeSeconds = 0.2f;

// The end screens' band runs from 240 to 460 (see EndScreen.cpp); the row of choices sits at its foot.
constexpr float kEndMenuY = kScreenHeight / 2.0f - 120.0f + 200.0f;
constexpr float kEndMenuSpread = 170.0f;

#ifdef __EMSCRIPTEN__
// In a browser the tab is closed, not the game, so there is nothing to quit.
constexpr std::array<const char*, 1> kTitleItems{"BEGIN"};
#else
constexpr std::array<const char*, 2> kTitleItems{"BEGIN", "QUIT"};
#endif
constexpr std::array<const char*, 2> kPauseItems{"RESUME", "RETURN TO TITLE"};
constexpr std::array<const char*, 2> kDeadItems{"TRY AGAIN", "RETURN TO TITLE"};
constexpr std::array<const char*, 2> kVictoryItems{"FIGHT AGAIN", "RETURN TO TITLE"};

Vec2 ItemCenter(const MenuLayout& layout, int index) {
    return layout.first + layout.step * static_cast<float>(index);
}

float ItemWidth(const char* label, float size, const Art& art) {
    return GameTextWidth(art.titleFont, label, size, kItemTracking);
}

// A line and a diamond on one side of the selected item, `side` -1 for the left and 1 for the right.
void DrawMark(Vec2 center, float halfWidth, float side, float grown, Color color) {
    const float diamondX = center.x + side * (halfWidth + kMarkGap);
    DrawPoly(Vector2{diamondX, center.y}, 4, kMarkDiamond, 0.0f, color);
    const float lineStart = diamondX + side * (kMarkDiamond + 4.0f);
    const float lineEnd = lineStart + side * kMarkLine * grown;
    DrawLineEx(Vector2{lineStart, center.y}, Vector2{lineEnd, center.y}, 1.0f, color);
}

} // namespace

std::span<const char* const> MenuLabelsFor(SessionState state) {
    switch (state) {
    case SessionState::Title:
        return kTitleItems;
    case SessionState::Paused:
        return kPauseItems;
    case SessionState::Dead:
        return kDeadItems;
    case SessionState::Victory:
        return kVictoryItems;
    case SessionState::Playing:
        break;
    }
    return {};
}

MenuLayout MenuLayoutFor(SessionState state) {
    switch (state) {
    case SessionState::Title:
        return MenuLayout{.first = {kMiddle, kTitleMenuY}, .step = {0.0f, kMenuSpacing}};
    case SessionState::Paused:
        return MenuLayout{.first = {kMiddle, kPauseMenuY}, .step = {0.0f, kMenuSpacing}};
    case SessionState::Dead:
    case SessionState::Victory:
        return MenuLayout{.first = {kMiddle - kEndMenuSpread, kEndMenuY}, .step = {2.0f * kEndMenuSpread, 0.0f},
                          .size = 16.0f};
    case SessionState::Playing:
        break;
    }
    return {};
}

std::optional<int> MenuItemAt(const MenuLayout& layout, std::span<const char* const> labels, Vec2 point,
                              const Art& art) {
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const Vec2 center = ItemCenter(layout, static_cast<int>(i));
        const float halfWidth = 0.5f * ItemWidth(labels[i], layout.size, art) + kHitPadX;
        const float halfHeight = 0.5f * layout.size + kHitPadY;
        if (std::abs(point.x - center.x) <= halfWidth && std::abs(point.y - center.y) <= halfHeight) {
            return static_cast<int>(i);
        }
    }
    return std::nullopt;
}

void DrawMenu(const MenuLayout& layout, std::span<const char* const> labels, const Menu& menu, float sinceMoved,
              float shown, const Art& art) {
    const float t = std::clamp(sinceMoved / kMarkSlideSeconds, 0.0f, 1.0f);
    const float grown = t * t * (3.0f - 2.0f * t);
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const bool selected = static_cast<int>(i) == menu.selected;
        const Vec2 center = ItemCenter(layout, static_cast<int>(i));
        const Color color = Fade(selected ? kText : kMenuIdle, shown);
        // The text is placed by its top; the letters are capitals, so their middle is half the size down.
        DrawGameTextCentered(art.titleFont, labels[i], center.x, center.y - 0.55f * layout.size, layout.size, color,
                             kItemTracking);
        if (selected) {
            const float halfWidth = 0.5f * ItemWidth(labels[i], layout.size, art);
            DrawMark(center, halfWidth, -1.0f, grown, color);
            DrawMark(center, halfWidth, 1.0f, grown, color);
        }
    }
}

void DrawTitleScreen(const Menu& menu, float sinceMoved, const Art& art) {
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(kBackground, 0.86f));
    DrawGameTextCentered(art.titleFont, "HORDE CARNAGE", kMiddle, kTitleY, kTitleSize, kText, kTitleTracking * 1.6f);
    DrawLineEx(Vector2{kMiddle - kTitleRuleHalf, kTitleRuleY}, Vector2{kMiddle + kTitleRuleHalf, kTitleRuleY}, 1.0f,
               kHudFrame);
    DrawMenu(MenuLayoutFor(SessionState::Title), MenuLabelsFor(SessionState::Title), menu, sinceMoved, 1.0f, art);
    DrawGameTextCentered(art.uiFont, "v" HORDE_VERSION, kMiddle, kScreenHeight - 34.0f, 13.0f, kMenuFaint);
}

void DrawPauseScreen(const Menu& menu, float sinceMoved, float seconds, const Art& art) {
    const float shown = std::clamp(seconds / kPauseFadeSeconds, 0.0f, 1.0f);
    DrawGameTextCentered(art.titleFont, "PAUSED", kMiddle, kPauseY, kPauseSize, Fade(kText, shown),
                         kTitleTracking * 2.0f);
    DrawMenu(MenuLayoutFor(SessionState::Paused), MenuLabelsFor(SessionState::Paused), menu, sinceMoved, shown,
             art);
}

} // namespace horde
