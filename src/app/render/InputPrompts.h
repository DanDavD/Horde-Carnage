#pragma once

#include "app/PlayerInput.h"
#include "app/render/Art.h"
#include "core/GamepadType.h"

namespace horde {

/// Active input family shown by on-screen button and key prompts.
struct InputPromptStyle {
    InputDevice device = InputDevice::KeyboardMouse;
    GamepadType gamepad = GamepadType::Generic;
};

/// Mouse glyphs drawn in code alongside keyboard keycaps.
enum class MouseGlyph {
    Move,
    LeftButton,
    Wheel,
};

/// Which menu screen's footer bar to draw. Settings splits value rows (`Change`) from action rows (`Select`).
enum class MenuFooterKind {
    Title,
    Pause,
    SettingsValue,
    SettingsChoice,
    Controls,
    EndRow,
};

/// Width in game pixels of a procedural gamepad icon (`control` drawn for `type`).
[[nodiscard]] float GamepadControlWidth(GamepadType type, GamepadControl control, const Art& art);

/// Draws a procedural gamepad icon with its left edge at `left` and vertical middle at `centerY`,
/// faded by `alpha` (0 to 1), and returns its width in game pixels.
float DrawGamepadControl(GamepadType type, GamepadControl control, float left, float centerY, float alpha,
                         const Art& art);

/// Width in game pixels of a procedural keyboard keycap with `label`.
[[nodiscard]] float KeyCapWidth(const char* label, const Art& art);

/// Draws a dark forged keycap with its left edge at `left` and vertical middle at `centerY`,
/// faded by `alpha`, and returns its width in game pixels.
float DrawKeyCap(const char* label, float left, float centerY, float alpha, const Art& art);

/// Width in game pixels of a procedural mouse icon.
[[nodiscard]] float MouseIconWidth();

/// Draws a procedural mouse icon (`Move`, `LeftButton` or `Wheel`) at `left`, `centerY` and returns its width.
float DrawMouseIcon(MouseGlyph glyph, float left, float centerY, float alpha);

/// Draws the centred prompt bar at the foot of a menu screen (`A Select  <>  B Back` or `[Enter] Select`,
/// matching `style`), faded by `alpha` (0 to 1).
void DrawMenuFooter(MenuFooterKind kind, InputPromptStyle style, float alpha, const Art& art);

/// Draws the live combat prompt strip along the bottom-left of the HUD (`Move`, `Aim`, `Fire`, `Dash`, `Heal`,
/// `Gun`, `Pause`), switching on the fly between keyboard/mouse and the active gamepad family.
void DrawHudPrompts(InputPromptStyle style, const Art& art);

} // namespace horde
