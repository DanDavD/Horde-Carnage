#include "app/render/MenuDraw.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>

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

// The settings and the controls: a heading and a rule at the top, then the lines.
constexpr float kHeadingSize = 40.0f;
constexpr float kHeadingY = 128.0f;
constexpr float kHeadingRuleY = kHeadingY + kHeadingSize + 22.0f;

// Each line of the settings: the name on the left and the value in a column on the right. The arrows on each
// side of the selected value sit at a fixed place, so they do not move as the value's width changes.
constexpr float kRowSize = 20.0f;
constexpr float kRowFirstY = 262.0f;
constexpr float kRowSpacing = 46.0f;
constexpr float kRowGap = 22.0f; // Between the values and Controls and Back.
constexpr float kRowHalf = 260.0f;
constexpr float kValueCenterX = kMiddle + 160.0f;
constexpr float kValueSize = 18.0f;
constexpr float kArrowOffset = 92.0f;
constexpr float kArrowSize = 6.0f;
constexpr float kArrowHitHalf = 18.0f;
// The volume, as ten marks lit up to the level.
constexpr float kTickSpacing = 10.0f;
constexpr float kTickWidth = 3.0f;
constexpr float kTickHeight = 14.0f;

// The controls: three columns, a line of headings and one line per action.
constexpr float kControlsLeft = kMiddle - 360.0f;
constexpr float kControlsKeysX = kMiddle - 100.0f;
constexpr float kControlsPadX = kMiddle + 230.0f;
constexpr float kControlsHeadY = 226.0f;
constexpr float kControlsFirstY = 262.0f;
constexpr float kControlsSpacing = 32.0f;
constexpr float kControlsSize = 16.0f;
constexpr float kControlsBackY = 590.0f;

// The end screens' band runs from 240 to 460 (see EndScreen.cpp); the row of choices sits at its foot.
constexpr float kEndMenuY = kScreenHeight / 2.0f - 120.0f + 200.0f;
constexpr float kEndMenuSpread = 170.0f;

#ifdef __EMSCRIPTEN__
// In a browser the tab is closed, not the game, so there is nothing to quit.
constexpr std::array<const char*, 2> kTitleItems{"BEGIN", "SETTINGS"};
// Nor a display or a frame rate to choose: the page decides both.
constexpr std::array<SettingsRow, 4> kSettingsRows{{
    {"MASTER VOLUME", SettingsRowKind::Value, Setting::MasterVolume},
    {"SCREEN SHAKE", SettingsRowKind::Value, Setting::ScreenShake},
    {"CONTROLS", SettingsRowKind::Controls},
    {"BACK", SettingsRowKind::Back},
}};
#else
constexpr std::array<const char*, 3> kTitleItems{"BEGIN", "SETTINGS", "QUIT"};
constexpr std::array<SettingsRow, 6> kSettingsRows{{
    {"MASTER VOLUME", SettingsRowKind::Value, Setting::MasterVolume},
    {"SCREEN SHAKE", SettingsRowKind::Value, Setting::ScreenShake},
    {"DISPLAY", SettingsRowKind::Value, Setting::Display},
    {"FRAME RATE", SettingsRowKind::Value, Setting::FrameRate},
    {"CONTROLS", SettingsRowKind::Controls},
    {"BACK", SettingsRowKind::Back},
}};
#endif
constexpr std::array<const char*, 3> kPauseItems{"RESUME", "SETTINGS", "RETURN TO TITLE"};
constexpr std::array<const char*, 2> kDeadItems{"TRY AGAIN", "RETURN TO TITLE"};
constexpr std::array<const char*, 2> kVictoryItems{"FIGHT AGAIN", "RETURN TO TITLE"};
constexpr std::array<const char*, 1> kControlsItems{"BACK"};

// One line of the controls table: the action, then the keyboard and mouse, then the gamepad.
struct ControlsLine {
    const char* action;
    const char* keys;
    const char* pad;
};

constexpr std::array<ControlsLine, 8> kControlsLines{{
    {"Move", "WASD or the arrows", "Left stick or d-pad"},
    {"Aim", "Mouse", "-"},
    {"Fire", "Left click, hold with the SMG", "-"},
    {"Switch gun", "Q, 1, 2 or the mouse wheel", "-"},
    {"Dash", "Space or left Shift", "-"},
    {"Drink a flask", "E", "-"},
    {"Pause", "Esc", "Start"},
    {"Try again at once", "R, after the fight", "-"},
}};

// Where the items of a page that is a plain list sit: the middle of the first one, and the step to each next
// one, down a column or along a row.
struct MenuLayout {
    Vec2 first;
    Vec2 step;
    float size = 22.0f; // The height of the letters.
};

std::span<const char* const> MenuLabelsFor(MenuPage page) {
    switch (page) {
    case MenuPage::Title:
        return kTitleItems;
    case MenuPage::Pause:
        return kPauseItems;
    case MenuPage::Dead:
        return kDeadItems;
    case MenuPage::Victory:
        return kVictoryItems;
    case MenuPage::Controls:
        return kControlsItems;
    case MenuPage::Settings:
    case MenuPage::None:
        break;
    }
    return {};
}

MenuLayout MenuLayoutFor(MenuPage page) {
    switch (page) {
    case MenuPage::Title:
        return MenuLayout{.first = {kMiddle, kTitleMenuY}, .step = {0.0f, kMenuSpacing}};
    case MenuPage::Pause:
        return MenuLayout{.first = {kMiddle, kPauseMenuY}, .step = {0.0f, kMenuSpacing}};
    case MenuPage::Dead:
    case MenuPage::Victory:
        return MenuLayout{.first = {kMiddle - kEndMenuSpread, kEndMenuY}, .step = {2.0f * kEndMenuSpread, 0.0f},
                          .size = 16.0f};
    case MenuPage::Controls:
        return MenuLayout{.first = {kMiddle, kControlsBackY}, .step = {0.0f, kMenuSpacing}};
    case MenuPage::Settings:
    case MenuPage::None:
        break;
    }
    return {};
}

// The middle of a line of the settings. Controls and Back sit a little apart from the values.
float SettingsRowY(std::size_t index) {
    const bool value = kSettingsRows[index].kind == SettingsRowKind::Value;
    return kRowFirstY + kRowSpacing * static_cast<float>(index) + (value ? 0.0f : kRowGap);
}

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

// Both marks of the selected item, whose lines slide out after a move.
void DrawMarks(Vec2 center, float halfWidth, float sinceMoved, Color color) {
    const float t = std::clamp(sinceMoved / kMarkSlideSeconds, 0.0f, 1.0f);
    const float grown = t * t * (3.0f - 2.0f * t);
    DrawMark(center, halfWidth, -1.0f, grown, color);
    DrawMark(center, halfWidth, 1.0f, grown, color);
}

void DrawHeading(const char* text, const Art& art) {
    DrawGameTextCentered(art.titleFont, text, kMiddle, kHeadingY, kHeadingSize, kText, kTitleTracking * 2.0f);
    DrawLineEx(Vector2{kMiddle - kTitleRuleHalf, kHeadingRuleY}, Vector2{kMiddle + kTitleRuleHalf, kHeadingRuleY},
               1.0f, kHudFrame);
}

// A small triangle pointing left (`side` -1) or right (1), with its point at `tip`.
void DrawArrow(Vec2 tip, float side, Color color) {
    const Vector2 point = ToRaylib(tip);
    const Vector2 top{tip.x - side * kArrowSize, tip.y - kArrowSize};
    const Vector2 bottom{tip.x - side * kArrowSize, tip.y + kArrowSize};
    // raylib fills a triangle only when its corners go counter-clockwise on the screen.
    if (side < 0.0f) {
        DrawTriangle(point, bottom, top, color);
    } else {
        DrawTriangle(point, top, bottom, color);
    }
}

void DrawVolumeTicks(Vec2 center, int volume, Color lit) {
    const float width = kTickSpacing * static_cast<float>(kMaxVolume - 1) + kTickWidth;
    for (int i = 0; i < kMaxVolume; ++i) {
        const float x = center.x - 0.5f * width + kTickSpacing * static_cast<float>(i);
        // Each mark a little taller than the last, so the row reads as rising loudness.
        const float height = kTickHeight * (0.55f + 0.45f * static_cast<float>(i) / (kMaxVolume - 1));
        const Color color = i < volume ? lit : kMenuFaint;
        DrawRectangleRec(Rectangle{x, center.y + 0.5f * kTickHeight - height, kTickWidth, height}, color);
    }
}

} // namespace

MenuPage PageFor(SessionState state) {
    switch (state) {
    case SessionState::Title:
        return MenuPage::Title;
    case SessionState::Paused:
        return MenuPage::Pause;
    case SessionState::Dead:
        return MenuPage::Dead;
    case SessionState::Victory:
        return MenuPage::Victory;
    case SessionState::Playing:
        break;
    }
    return MenuPage::None;
}

int MenuItemCount(MenuPage page) {
    if (page == MenuPage::Settings) {
        return static_cast<int>(kSettingsRows.size());
    }
    return static_cast<int>(MenuLabelsFor(page).size());
}

bool IsRowMenu(MenuPage page) {
    return page == MenuPage::Dead || page == MenuPage::Victory;
}

std::span<const SettingsRow> SettingsRows() {
    return kSettingsRows;
}

std::optional<MenuTarget> MenuTargetAt(MenuPage page, Vec2 point, const Art& art) {
    if (page == MenuPage::Settings) {
        for (std::size_t i = 0; i < kSettingsRows.size(); ++i) {
            const bool onLine = std::abs(point.x - kMiddle) <= kRowHalf + kHitPadX &&
                                std::abs(point.y - SettingsRowY(i)) <= 0.5f * kRowSize + kHitPadY;
            if (!onLine) {
                continue;
            }
            const bool value = kSettingsRows[i].kind == SettingsRowKind::Value;
            const float lowerArrowX = kValueCenterX - kArrowOffset;
            return MenuTarget{.item = static_cast<int>(i),
                              .lowerArrow = value && std::abs(point.x - lowerArrowX) <= kArrowHitHalf};
        }
        return std::nullopt;
    }
    const std::span<const char* const> labels = MenuLabelsFor(page);
    const MenuLayout layout = MenuLayoutFor(page);
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const Vec2 center = ItemCenter(layout, static_cast<int>(i));
        const float halfWidth = 0.5f * ItemWidth(labels[i], layout.size, art) + kHitPadX;
        const float halfHeight = 0.5f * layout.size + kHitPadY;
        if (std::abs(point.x - center.x) <= halfWidth && std::abs(point.y - center.y) <= halfHeight) {
            return MenuTarget{.item = static_cast<int>(i)};
        }
    }
    return std::nullopt;
}

void DrawMenu(MenuPage page, const Menu& menu, float sinceMoved, float shown, const Art& art) {
    const std::span<const char* const> labels = MenuLabelsFor(page);
    const MenuLayout layout = MenuLayoutFor(page);
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const bool selected = static_cast<int>(i) == menu.selected;
        const Vec2 center = ItemCenter(layout, static_cast<int>(i));
        const Color color = Fade(selected ? kText : kMenuIdle, shown);
        // The text is placed by its top; the letters are capitals, so their middle is half the size down.
        DrawGameTextCentered(art.titleFont, labels[i], center.x, center.y - 0.55f * layout.size, layout.size, color,
                             kItemTracking);
        if (selected) {
            DrawMarks(center, 0.5f * ItemWidth(labels[i], layout.size, art), sinceMoved, color);
        }
    }
}

void DrawTitleShade() {
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(kBackground, 0.86f));
}

void DrawPageShade() {
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(kBackground, 0.6f));
}

void DrawTitleScreen(const Menu& menu, float sinceMoved, const Art& art) {
    DrawTitleShade();
    DrawGameTextCentered(art.titleFont, "HORDE CARNAGE", kMiddle, kTitleY, kTitleSize, kText, kTitleTracking * 1.6f);
    DrawLineEx(Vector2{kMiddle - kTitleRuleHalf, kTitleRuleY}, Vector2{kMiddle + kTitleRuleHalf, kTitleRuleY}, 1.0f,
               kHudFrame);
    DrawMenu(MenuPage::Title, menu, sinceMoved, 1.0f, art);
    DrawGameTextCentered(art.uiFont, "v" HORDE_VERSION, kMiddle, kScreenHeight - 34.0f, 13.0f, kMenuFaint);
}

void DrawPauseScreen(const Menu& menu, float sinceMoved, float seconds, const Art& art) {
    const float shown = std::clamp(seconds / kPauseFadeSeconds, 0.0f, 1.0f);
    DrawGameTextCentered(art.titleFont, "PAUSED", kMiddle, kPauseY, kPauseSize, Fade(kText, shown),
                         kTitleTracking * 2.0f);
    DrawMenu(MenuPage::Pause, menu, sinceMoved, shown, art);
}

void DrawSettingsScreen(const Menu& menu, float sinceMoved, const Settings& settings, const Art& art) {
    DrawHeading("SETTINGS", art);
    for (std::size_t i = 0; i < kSettingsRows.size(); ++i) {
        const SettingsRow& row = kSettingsRows[i];
        const bool selected = static_cast<int>(i) == menu.selected;
        const Color color = selected ? kText : kMenuIdle;
        const Vec2 center{kMiddle, SettingsRowY(i)};
        DrawGameText(art.titleFont, row.label, Vec2{kMiddle - kRowHalf, center.y - 0.55f * kRowSize}, kRowSize,
                     color, kItemTracking);
        if (selected) {
            DrawMarks(center, kRowHalf, sinceMoved, color);
        }
        if (row.kind != SettingsRowKind::Value) {
            continue;
        }
        const Vec2 value{kValueCenterX, center.y};
        if (row.setting == Setting::MasterVolume) {
            DrawVolumeTicks(value, settings.masterVolume, color);
        } else {
            const std::string text = SettingValueText(settings, row.setting);
            DrawGameTextCentered(art.titleFont, text.c_str(), value.x, value.y - 0.55f * kValueSize, kValueSize,
                                 color, kItemTracking);
        }
        if (selected) {
            DrawArrow(Vec2{value.x - kArrowOffset, value.y}, -1.0f, color);
            DrawArrow(Vec2{value.x + kArrowOffset, value.y}, 1.0f, color);
        }
    }
}

void DrawControlsScreen(const Menu& menu, float sinceMoved, const Art& art) {
    DrawHeading("CONTROLS", art);
    const auto drawLine = [&art](float y, const ControlsLine& line, float size, Color first, Color rest) {
        DrawGameText(art.uiFont, line.action, Vec2{kControlsLeft, y}, size, first);
        DrawGameText(art.uiFont, line.keys, Vec2{kControlsKeysX, y}, size, rest);
        DrawGameText(art.uiFont, line.pad, Vec2{kControlsPadX, y}, size, rest);
    };
    drawLine(kControlsHeadY, ControlsLine{"ACTION", "KEYBOARD AND MOUSE", "GAMEPAD"}, 12.0f, kMenuFaint, kMenuFaint);
    for (std::size_t i = 0; i < kControlsLines.size(); ++i) {
        drawLine(kControlsFirstY + kControlsSpacing * static_cast<float>(i), kControlsLines[i], kControlsSize, kText,
                 kSoftText);
    }
    DrawMenu(MenuPage::Controls, menu, sinceMoved, 1.0f, art);
}

} // namespace horde
