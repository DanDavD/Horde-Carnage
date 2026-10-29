#include "app/render/InputPrompts.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include <raylib.h>

#include "app/RaylibConvert.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/Shapes.h"
#include "app/render/Text.h"
#include "app/render/View.h"

namespace horde {

namespace {

constexpr float kFaceDiameter = 20.0f;
constexpr float kFaceRadius = 9.5f;
constexpr float kFaceLabelSize = 12.5f;
constexpr float kShoulderHeight = 17.5f;
constexpr float kShoulderLabelSize = 11.5f;
constexpr float kShoulderPadX = 11.0f;
constexpr float kShoulderMinWidth = 24.0f;
constexpr float kKeyCapHeight = 18.0f;
constexpr float kKeyCapLabelSize = 11.0f;
constexpr float kKeyCapPadX = 11.0f;
constexpr float kKeyCapMinWidth = 20.0f;
constexpr float kMouseWidth = 14.0f;
constexpr float kMouseHeight = 18.0f;

constexpr float kBadgePairGap = 4.0f;
constexpr float kLabelGap = 6.0f;
constexpr float kFooterLabelSize = 13.5f;
constexpr float kFooterGroupGap = 28.0f;
constexpr float kFooterCenterY = kScreenHeight - 28.0f;

constexpr float kHudMarginX = 20.0f;
constexpr float kHudCenterY = kScreenHeight - 26.0f;
constexpr float kHudLabelSize = 13.0f;
constexpr float kHudGroupGap = 15.0f;

enum class BadgeKind {
    Gamepad,
    Key,
    Mouse,
};

struct PromptBadge {
    BadgeKind kind = BadgeKind::Key;
    GamepadControl pad = GamepadControl::FaceSouth;
    const char* key = "";
    MouseGlyph mouse = MouseGlyph::Move;
};

struct PromptGroup {
    std::array<PromptBadge, 2> badges{};
    int badgeCount = 1;
    const char* label = "";
    bool slashBetween = false;
};

constexpr PromptBadge PadBadge(GamepadControl control) {
    return PromptBadge{.kind = BadgeKind::Gamepad, .pad = control};
}

constexpr PromptBadge KeyBadge(const char* key) {
    return PromptBadge{.kind = BadgeKind::Key, .key = key};
}

constexpr PromptBadge MouseBadge(MouseGlyph mouse) {
    return PromptBadge{.kind = BadgeKind::Mouse, .mouse = mouse};
}

constexpr PromptGroup OneBadge(PromptBadge badge, const char* label) {
    return PromptGroup{.badges = {badge, PromptBadge{}}, .badgeCount = 1, .label = label};
}

constexpr PromptGroup TwoBadges(PromptBadge first, PromptBadge second, const char* label, bool slash = false) {
    return PromptGroup{.badges = {first, second}, .badgeCount = 2, .label = label, .slashBetween = slash};
}

bool IsShoulderControl(GamepadControl control) {
    return control == GamepadControl::LeftBumper || control == GamepadControl::RightBumper ||
           control == GamepadControl::LeftTrigger || control == GamepadControl::RightTrigger;
}

Color XboxAccent(GamepadControl control) {
    switch (control) {
    case GamepadControl::FaceSouth:
        return kXboxA;
    case GamepadControl::FaceEast:
        return kXboxB;
    case GamepadControl::FaceWest:
        return kXboxX;
    case GamepadControl::FaceNorth:
        return kXboxY;
    default:
        return kHudAccent;
    }
}

Color PlayStationAccent(GamepadControl control) {
    switch (control) {
    case GamepadControl::FaceSouth:
        return kPsCross;
    case GamepadControl::FaceEast:
        return kPsCircle;
    case GamepadControl::FaceWest:
        return kPsSquare;
    case GamepadControl::FaceNorth:
        return kPsTriangle;
    default:
        return kHudAccent;
    }
}

void DrawPlayStationSymbol(GamepadControl control, Vector2 center, float alpha) {
    const Color color = Fade(PlayStationAccent(control), alpha);
    switch (control) {
    case GamepadControl::FaceSouth: {
        constexpr float d = 4.1f;
        DrawLineEx(Vector2{center.x - d, center.y - d}, Vector2{center.x + d, center.y + d}, 2.0f, color);
        DrawLineEx(Vector2{center.x - d, center.y + d}, Vector2{center.x + d, center.y - d}, 2.0f, color);
        break;
    }
    case GamepadControl::FaceEast:
        DrawRing(center, 3.5f, 5.2f, 0.0f, 360.0f, 24, color);
        break;
    case GamepadControl::FaceWest:
        DrawRectangleLinesEx(Rectangle{center.x - 4.3f, center.y - 4.3f, 8.6f, 8.6f}, 1.8f, color);
        break;
    case GamepadControl::FaceNorth: {
        const Vector2 top{center.x, center.y - 4.9f};
        const Vector2 left{center.x - 4.6f, center.y + 3.5f};
        const Vector2 right{center.x + 4.6f, center.y + 3.5f};
        DrawLineEx(top, left, 1.8f, color);
        DrawLineEx(left, right, 1.8f, color);
        DrawLineEx(right, top, 1.8f, color);
        break;
    }
    default:
        break;
    }
}

void DrawFaceButton(GamepadType type, GamepadControl control, Vector2 center, float alpha, const Art& art) {
    DrawCircleV(Vector2{center.x, center.y + 1.0f}, kFaceRadius + 0.6f, Fade(BLACK, 0.55f * alpha));
    DrawCircleV(center, kFaceRadius, Fade(kPromptFill, alpha));

    Color rim = kHudFrame;
    if (type == GamepadType::Xbox) {
        rim = XboxAccent(control);
    } else if (type == GamepadType::PlayStation) {
        rim = Mix(kHudFrame, PlayStationAccent(control), 0.45f);
    } else if (type == GamepadType::Switch) {
        rim = kHudAccent;
    } else {
        // Generic pads use neutral iron-and-bone rims without brand color coding.
        rim = Mix(kHudFrame, kHudAccent, 0.5f);
    }
    DrawRing(center, kFaceRadius - 1.5f, kFaceRadius, 0.0f, 360.0f, 28, Fade(rim, alpha));

    if (type == GamepadType::PlayStation) {
        DrawPlayStationSymbol(control, center, alpha);
        return;
    }

    const char* letter = GamepadControlLabel(type, control);
    Color textColor = kText;
    if (type == GamepadType::Xbox) {
        textColor = Mix(kText, XboxAccent(control), 0.28f);
    }
    DrawGameTextCentered(art.uiFont, letter, center.x, center.y - 0.54f * kFaceLabelSize, kFaceLabelSize,
                         Fade(textColor, alpha), 0.0f);
}

void DrawShoulderButton(GamepadType type, GamepadControl control, float left, float centerY, float width, float alpha,
                        const Art& art) {
    const bool trigger = control == GamepadControl::LeftTrigger || control == GamepadControl::RightTrigger;
    const float height = trigger ? kShoulderHeight + 1.0f : kShoulderHeight;
    const float top = centerY - 0.5f * height;
    const Rectangle shadow{left, top + 1.0f, width, height};
    const Rectangle box{left, top, width, height};
    const float roundness = trigger ? 0.36f : 0.55f;

    DrawRectangleRounded(shadow, roundness, 6, Fade(BLACK, 0.55f * alpha));
    DrawRectangleRounded(box, roundness, 6, Fade(kPromptFill, alpha));
    if (trigger) {
        // A lit lip along the top distinguishes lower triggers from flat upper bumpers at a glance.
        DrawRectangleRec(Rectangle{left + 3.0f, top + 1.0f, width - 6.0f, 2.0f}, Fade(kHudAccent, 0.5f * alpha));
    }
    DrawRectangleRoundedLinesEx(box, roundness, 6, 1.3f, Fade(kHudAccent, 0.9f * alpha));

    const char* label = GamepadControlLabel(type, control);
    DrawGameTextCentered(art.uiFont, label, left + 0.5f * width, centerY - 0.53f * kShoulderLabelSize,
                         kShoulderLabelSize, Fade(kText, alpha), 0.02f);
}

void DrawStartButton(GamepadType type, float left, float centerY, float alpha) {
    if (type == GamepadType::PlayStation) {
        // DualShock 4 / DualSense Options capsule, narrow and vertical.
        constexpr float kWidth = 14.0f;
        constexpr float kHeight = 18.0f;
        const Rectangle box{left, centerY - 0.5f * kHeight, kWidth, kHeight};
        DrawRectangleRounded(Rectangle{box.x, box.y + 1.0f, box.width, box.height}, 0.8f, 6,
                             Fade(BLACK, 0.55f * alpha));
        DrawRectangleRounded(box, 0.8f, 6, Fade(kPromptFill, alpha));
        DrawRectangleRoundedLinesEx(box, 0.8f, 6, 1.3f, Fade(kHudAccent, alpha));
        const float cx = left + 0.5f * kWidth;
        for (const float dy : {-3.0f, 0.0f, 3.0f}) {
            DrawLineEx(Vector2{cx - 2.6f, centerY + dy}, Vector2{cx + 2.6f, centerY + dy}, 1.3f,
                       Fade(kText, alpha));
        }
        return;
    }

    if (type == GamepadType::Generic) {
        // Universal generic pad Start pill with a right-pointing triangle.
        constexpr float kWidth = 20.0f;
        constexpr float kHeight = 16.0f;
        const Rectangle box{left, centerY - 0.5f * kHeight, kWidth, kHeight};
        DrawRectangleRounded(Rectangle{box.x, box.y + 1.0f, box.width, box.height}, 0.6f, 6,
                             Fade(BLACK, 0.55f * alpha));
        DrawRectangleRounded(box, 0.6f, 6, Fade(kPromptFill, alpha));
        DrawRectangleRoundedLinesEx(box, 0.6f, 6, 1.3f, Fade(Mix(kHudFrame, kHudAccent, 0.5f), alpha));
        const float cx = left + 0.5f * kWidth;
        DrawTriangleAnyWinding(Vec2{cx - 2.6f, centerY - 3.6f}, Vec2{cx - 2.6f, centerY + 3.6f},
                               Vec2{cx + 3.6f, centerY}, Fade(kText, alpha));
        return;
    }

    // Xbox Menu (three horizontal lines) and Switch Plus (+), both circular.
    constexpr float kRadius = 8.5f;
    const Vector2 center{left + kRadius + 0.5f, centerY};
    DrawCircleV(Vector2{center.x, center.y + 1.0f}, kRadius + 0.5f, Fade(BLACK, 0.55f * alpha));
    DrawCircleV(center, kRadius, Fade(kPromptFill, alpha));
    DrawRing(center, kRadius - 1.4f, kRadius, 0.0f, 360.0f, 24, Fade(kHudAccent, alpha));

    if (type == GamepadType::Switch) {
        DrawLineEx(Vector2{center.x - 3.8f, center.y}, Vector2{center.x + 3.8f, center.y}, 1.8f, Fade(kText, alpha));
        DrawLineEx(Vector2{center.x, center.y - 3.8f}, Vector2{center.x, center.y + 3.8f}, 1.8f, Fade(kText, alpha));
    } else {
        for (const float dy : {-2.8f, 0.0f, 2.8f}) {
            DrawLineEx(Vector2{center.x - 3.8f, center.y + dy}, Vector2{center.x + 3.8f, center.y + dy}, 1.4f,
                       Fade(kText, alpha));
        }
    }
}

void DrawStickIcon(GamepadControl control, float left, float centerY, float alpha, const Art& art) {
    const Vector2 center{left + 10.0f, centerY};
    DrawCircleV(Vector2{center.x, center.y + 1.0f}, 9.8f, Fade(BLACK, 0.55f * alpha));
    DrawRing(center, 8.6f, 9.8f, 0.0f, 360.0f, 24, Fade(kHudFrame, 0.85f * alpha));
    const Vector2 cap{center.x, center.y - 0.5f};
    DrawCircleV(cap, 7.6f, Fade(kPromptFill, alpha));
    DrawRing(cap, 6.3f, 7.6f, 0.0f, 360.0f, 24, Fade(kHudAccent, alpha));
    const char* label = control == GamepadControl::LeftStick ? "L" : "R";
    DrawGameTextCentered(art.uiFont, label, cap.x, cap.y - 0.54f * 11.0f, 11.0f, Fade(kText, alpha), 0.0f);
}

void DrawDpadIcon(GamepadControl control, float left, float centerY, float alpha) {
    const Vector2 c{left + 10.0f, centerY};
    constexpr float kArm = 8.0f;
    constexpr float kThick = 5.4f;
    const bool litVert = control == GamepadControl::Dpad || control == GamepadControl::DpadVertical;
    const bool litHoriz = control == GamepadControl::Dpad || control == GamepadControl::DpadHorizontal;

    // Backing cross + border.
    DrawRectangleRec(Rectangle{c.x - 0.5f * kThick - 1.0f, c.y - kArm - 1.0f, kThick + 2.0f, 2.0f * kArm + 2.0f},
                     Fade(kHudFrame, 0.9f * alpha));
    DrawRectangleRec(Rectangle{c.x - kArm - 1.0f, c.y - 0.5f * kThick - 1.0f, 2.0f * kArm + 2.0f, kThick + 2.0f},
                     Fade(kHudFrame, 0.9f * alpha));
    DrawRectangleRec(Rectangle{c.x - 0.5f * kThick, c.y - kArm, kThick, 2.0f * kArm}, Fade(kPromptFill, alpha));
    DrawRectangleRec(Rectangle{c.x - kArm, c.y - 0.5f * kThick, 2.0f * kArm, kThick}, Fade(kPromptFill, alpha));

    const Color vColor = Fade(litVert ? kText : kMenuFaint, alpha);
    const Color hColor = Fade(litHoriz ? kText : kMenuFaint, alpha);
    DrawRectangleRec(Rectangle{c.x - 1.7f, c.y - kArm + 1.2f, 3.4f, 4.2f}, vColor);
    DrawRectangleRec(Rectangle{c.x - 1.7f, c.y + kArm - 5.4f, 3.4f, 4.2f}, vColor);
    DrawRectangleRec(Rectangle{c.x - kArm + 1.2f, c.y - 1.7f, 4.2f, 3.4f}, hColor);
    DrawRectangleRec(Rectangle{c.x + kArm - 5.4f, c.y - 1.7f, 4.2f, 3.4f}, hColor);
}

float BadgeWidth(const PromptBadge& badge, GamepadType gamepad, const Art& art) {
    switch (badge.kind) {
    case BadgeKind::Gamepad:
        return GamepadControlWidth(gamepad, badge.pad, art);
    case BadgeKind::Key:
        return KeyCapWidth(badge.key, art);
    case BadgeKind::Mouse:
        return MouseIconWidth();
    }
    return 0.0f;
}

float DrawBadge(const PromptBadge& badge, GamepadType gamepad, float left, float centerY, float alpha, const Art& art) {
    switch (badge.kind) {
    case BadgeKind::Gamepad:
        return DrawGamepadControl(gamepad, badge.pad, left, centerY, alpha, art);
    case BadgeKind::Key:
        return DrawKeyCap(badge.key, left, centerY, alpha, art);
    case BadgeKind::Mouse:
        return DrawMouseIcon(badge.mouse, left, centerY, alpha);
    }
    return 0.0f;
}

float GroupWidth(const PromptGroup& group, GamepadType gamepad, float labelSize, const Art& art) {
    float width = BadgeWidth(group.badges[0], gamepad, art);
    if (group.badgeCount > 1) {
        if (group.slashBetween) {
            width += 3.0f + GameTextWidth(art.uiFont, "/", labelSize) + 3.0f;
        } else {
            width += kBadgePairGap;
        }
        width += BadgeWidth(group.badges[1], gamepad, art);
    }
    width += kLabelGap + GameTextWidth(art.uiFont, group.label, labelSize);
    return width;
}

float DrawGroup(const PromptGroup& group, GamepadType gamepad, float left, float centerY, float labelSize,
                Color labelColor, float alpha, const Art& art) {
    float x = left;
    x += DrawBadge(group.badges[0], gamepad, x, centerY, alpha, art);
    if (group.badgeCount > 1) {
        if (group.slashBetween) {
            x += 3.0f;
            DrawGameText(art.uiFont, "/", Vec2{x, centerY - 0.52f * labelSize}, labelSize,
                         Fade(kMenuIdle, alpha));
            x += GameTextWidth(art.uiFont, "/", labelSize) + 3.0f;
        } else {
            x += kBadgePairGap;
        }
        x += DrawBadge(group.badges[1], gamepad, x, centerY, alpha, art);
    }
    x += kLabelGap;
    DrawGameText(art.uiFont, group.label, Vec2{x, centerY - 0.52f * labelSize}, labelSize,
                 Fade(labelColor, alpha));
    x += GameTextWidth(art.uiFont, group.label, labelSize);
    return x - left;
}

} // namespace

float GamepadControlWidth(GamepadType type, GamepadControl control, const Art& art) {
    if (IsShoulderControl(control)) {
        const char* label = GamepadControlLabel(type, control);
        return std::max(kShoulderMinWidth, GameTextWidth(art.uiFont, label, kShoulderLabelSize, 0.02f) + kShoulderPadX);
    }
    if (control == GamepadControl::Start) {
        if (type == GamepadType::PlayStation) {
            return 14.0f;
        }
        if (type == GamepadType::Generic) {
            return 20.0f;
        }
        return 18.0f;
    }
    return kFaceDiameter;
}

float DrawGamepadControl(GamepadType type, GamepadControl control, float left, float centerY, float alpha,
                         const Art& art) {
    const float width = GamepadControlWidth(type, control, art);
    if (alpha <= 0.0f) {
        return width;
    }
    if (IsShoulderControl(control)) {
        DrawShoulderButton(type, control, left, centerY, width, alpha, art);
        return width;
    }
    if (control == GamepadControl::Start) {
        DrawStartButton(type, left, centerY, alpha);
        return width;
    }
    if (control == GamepadControl::LeftStick || control == GamepadControl::RightStick) {
        DrawStickIcon(control, left, centerY, alpha, art);
        return width;
    }
    if (control == GamepadControl::Dpad || control == GamepadControl::DpadVertical ||
        control == GamepadControl::DpadHorizontal) {
        DrawDpadIcon(control, left, centerY, alpha);
        return width;
    }
    DrawFaceButton(type, control, Vector2{left + 0.5f * width, centerY}, alpha, art);
    return width;
}

float KeyCapWidth(const char* label, const Art& art) {
    return std::max(kKeyCapMinWidth, GameTextWidth(art.uiFont, label, kKeyCapLabelSize, 0.03f) + kKeyCapPadX);
}

float DrawKeyCap(const char* label, float left, float centerY, float alpha, const Art& art) {
    const float width = KeyCapWidth(label, art);
    if (alpha <= 0.0f) {
        return width;
    }
    const float top = centerY - 0.5f * kKeyCapHeight;
    const Rectangle bevel{left, top + 1.5f, width, kKeyCapHeight};
    const Rectangle face{left, top, width, kKeyCapHeight - 1.0f};
    DrawRectangleRounded(bevel, 0.28f, 4, Fade(BLACK, 0.65f * alpha));
    DrawRectangleRounded(face, 0.28f, 4, Fade(kPromptFill, alpha));
    DrawRectangleRoundedLinesEx(face, 0.28f, 4, 1.2f, Fade(Mix(kHudFrame, kHudAccent, 0.55f), alpha));
    DrawGameTextCentered(art.uiFont, label, left + 0.5f * width, centerY - 0.5f - 0.53f * kKeyCapLabelSize,
                         kKeyCapLabelSize, Fade(kText, alpha), 0.03f);
    return width;
}

float MouseIconWidth() {
    return kMouseWidth;
}

float DrawMouseIcon(MouseGlyph glyph, float left, float centerY, float alpha) {
    if (alpha <= 0.0f) {
        return kMouseWidth;
    }
    const float top = centerY - 0.5f * kMouseHeight;
    const Rectangle body{left, top, kMouseWidth, kMouseHeight};
    DrawRectangleRounded(Rectangle{body.x, body.y + 1.0f, body.width, body.height}, 0.75f, 6,
                         Fade(BLACK, 0.55f * alpha));
    DrawRectangleRounded(body, 0.75f, 6, Fade(kPromptFill, alpha));
    if (glyph == MouseGlyph::LeftButton) {
        // Highlight the top-left button quadrant for left-click fire.
        DrawRectangleRec(Rectangle{left + 2.0f, top + 2.0f, 0.5f * kMouseWidth - 2.0f, 6.5f},
                         Fade(kHudAccent, 0.85f * alpha));
    }
    DrawRectangleRoundedLinesEx(body, 0.75f, 6, 1.2f, Fade(kHudAccent, alpha));
    const float midX = left + 0.5f * kMouseWidth;
    DrawLineEx(Vector2{midX, top + 1.5f}, Vector2{midX, top + 8.5f}, 1.0f, Fade(kHudFrame, alpha));
    DrawLineEx(Vector2{left + 1.5f, top + 8.5f}, Vector2{left + kMouseWidth - 1.5f, top + 8.5f}, 1.0f,
               Fade(kHudFrame, alpha));
    if (glyph == MouseGlyph::Wheel) {
        DrawRectangleRec(Rectangle{midX - 1.2f, top + 3.0f, 2.4f, 4.2f}, Fade(kText, alpha));
    }
    return kMouseWidth;
}

void DrawMenuFooter(MenuFooterKind kind, InputPromptStyle style, float alpha, const Art& art) {
    if (alpha <= 0.0f) {
        return;
    }

    std::array<PromptGroup, 3> groups{};
    std::size_t count = 0;

    if (style.device == InputDevice::Gamepad) {
        switch (kind) {
        case MenuFooterKind::Title:
            groups[count++] = TwoBadges(PadBadge(GamepadControl::LeftStick),
                                        PadBadge(GamepadControl::DpadVertical), "Move");
            groups[count++] = OneBadge(PadBadge(GamepadControl::FaceSouth), "Select");
            break;
        case MenuFooterKind::Pause:
            groups[count++] = TwoBadges(PadBadge(GamepadControl::LeftStick),
                                        PadBadge(GamepadControl::DpadVertical), "Move");
            groups[count++] = OneBadge(PadBadge(GamepadControl::FaceSouth), "Select");
            groups[count++] = OneBadge(PadBadge(GamepadControl::FaceEast), "Resume");
            break;
        case MenuFooterKind::SettingsValue:
            groups[count++] = TwoBadges(PadBadge(GamepadControl::LeftStick),
                                        PadBadge(GamepadControl::DpadVertical), "Move");
            groups[count++] = TwoBadges(PadBadge(GamepadControl::LeftStick),
                                        PadBadge(GamepadControl::DpadHorizontal), "Change");
            groups[count++] = OneBadge(PadBadge(GamepadControl::FaceEast), "Back");
            break;
        case MenuFooterKind::SettingsChoice:
            groups[count++] = TwoBadges(PadBadge(GamepadControl::LeftStick),
                                        PadBadge(GamepadControl::DpadVertical), "Move");
            groups[count++] = OneBadge(PadBadge(GamepadControl::FaceSouth), "Select");
            groups[count++] = OneBadge(PadBadge(GamepadControl::FaceEast), "Back");
            break;
        case MenuFooterKind::Controls:
            groups[count++] = TwoBadges(PadBadge(GamepadControl::FaceSouth),
                                        PadBadge(GamepadControl::FaceEast), "Back", true);
            break;
        case MenuFooterKind::EndRow:
            groups[count++] = TwoBadges(PadBadge(GamepadControl::LeftStick),
                                        PadBadge(GamepadControl::DpadHorizontal), "Move");
            groups[count++] = OneBadge(PadBadge(GamepadControl::FaceSouth), "Select");
            break;
        }
    } else {
        switch (kind) {
        case MenuFooterKind::Title:
            groups[count++] = OneBadge(KeyBadge("W/S"), "Move");
            groups[count++] = OneBadge(KeyBadge("ENTER"), "Select");
            break;
        case MenuFooterKind::Pause:
            groups[count++] = OneBadge(KeyBadge("W/S"), "Move");
            groups[count++] = OneBadge(KeyBadge("ENTER"), "Select");
            groups[count++] = OneBadge(KeyBadge("ESC"), "Resume");
            break;
        case MenuFooterKind::SettingsValue:
            groups[count++] = OneBadge(KeyBadge("W/S"), "Move");
            groups[count++] = OneBadge(KeyBadge("A/D"), "Change");
            groups[count++] = OneBadge(KeyBadge("ESC"), "Back");
            break;
        case MenuFooterKind::SettingsChoice:
            groups[count++] = OneBadge(KeyBadge("W/S"), "Move");
            groups[count++] = OneBadge(KeyBadge("ENTER"), "Select");
            groups[count++] = OneBadge(KeyBadge("ESC"), "Back");
            break;
        case MenuFooterKind::Controls:
            groups[count++] = TwoBadges(KeyBadge("ENTER"), KeyBadge("ESC"), "Back", true);
            break;
        case MenuFooterKind::EndRow:
            groups[count++] = OneBadge(KeyBadge("A/D"), "Move");
            groups[count++] = OneBadge(KeyBadge("ENTER"), "Select");
            groups[count++] = OneBadge(KeyBadge("R"), "Retry");
            break;
        }
    }

    float totalWidth = 0.0f;
    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) {
            totalWidth += kFooterGroupGap;
        }
        totalWidth += GroupWidth(groups[i], style.gamepad, kFooterLabelSize, art);
    }

    float x = 0.5f * (static_cast<float>(kScreenWidth) - totalWidth);
    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) {
            // A small diamond between prompt groups echoes the selection marks of the menu above.
            DrawPoly(Vector2{x + 0.5f * kFooterGroupGap, kFooterCenterY}, 4, 2.5f, 0.0f,
                     Fade(kHudFrame, 0.85f * alpha));
            x += kFooterGroupGap;
        }
        x += DrawGroup(groups[i], style.gamepad, x, kFooterCenterY, kFooterLabelSize, kSoftText, alpha, art);
    }
}

void DrawHudPrompts(InputPromptStyle style, const Art& art) {
    std::array<PromptGroup, 8> groups{};
    std::size_t count = 0;

    if (style.device == InputDevice::Gamepad) {
        groups[count++] = OneBadge(PadBadge(GamepadControl::LeftStick), "Move");
        groups[count++] = OneBadge(PadBadge(GamepadControl::RightStick), "Aim");
        groups[count++] = OneBadge(PadBadge(GamepadControl::RightTrigger), "Fire");
        groups[count++] = TwoBadges(PadBadge(GamepadControl::LeftTrigger),
                                    PadBadge(GamepadControl::RightBumper), "Dash", true);
        groups[count++] = OneBadge(PadBadge(GamepadControl::FaceWest), "Heal");
        groups[count++] = OneBadge(PadBadge(GamepadControl::FaceNorth), "Gun");
        groups[count++] = OneBadge(PadBadge(GamepadControl::Start), "Pause");
    } else {
        groups[count++] = OneBadge(KeyBadge("WASD"), "Move");
        groups[count++] = OneBadge(MouseBadge(MouseGlyph::Move), "Aim");
        groups[count++] = OneBadge(MouseBadge(MouseGlyph::LeftButton), "Fire");
        groups[count++] = OneBadge(KeyBadge("SPACE"), "Dash");
        groups[count++] = OneBadge(KeyBadge("E"), "Heal");
        groups[count++] = OneBadge(KeyBadge("Q"), "Gun");
        groups[count++] = OneBadge(KeyBadge("ESC"), "Pause");
    }
#ifndef NDEBUG
    groups[count++] = OneBadge(KeyBadge("F1-F5"), "Debug");
#endif

    float x = kHudMarginX;
    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) {
            x += kHudGroupGap;
        }
        x += DrawGroup(groups[i], style.gamepad, x, kHudCenterY, kHudLabelSize, kSoftText, 1.0f, art);
    }
}

} // namespace horde
