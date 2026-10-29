#pragma once

#include <string_view>

namespace horde {

/// Visual family of a connected gamepad, used to pick the matching button glyphs and labels.
enum class GamepadType {
    Xbox,        ///< Xbox 360, Xbox One, Xbox Series and XInput pads.
    PlayStation, ///< DualShock 3/4, DualSense and PlayStation pads.
    Switch,      ///< Nintendo Switch Pro Controller and Joy-Con pads.
    Generic,     ///< Any unrecognized pad; uses positional A/B/X/Y labels without brand colors.
};

/// Logical buttons and sticks shown in prompts, named by physical position on the pad.
enum class GamepadControl {
    FaceSouth,      ///< Bottom face button: Xbox A, PlayStation Cross, Switch B.
    FaceEast,       ///< Right face button: Xbox B, PlayStation Circle, Switch A.
    FaceWest,       ///< Left face button: Xbox X, PlayStation Square, Switch Y.
    FaceNorth,      ///< Top face button: Xbox Y, PlayStation Triangle, Switch X.
    LeftBumper,     ///< Upper left shoulder: LB, L1, L.
    RightBumper,    ///< Upper right shoulder: RB, R1, R.
    LeftTrigger,    ///< Lower left trigger: LT, L2, ZL.
    RightTrigger,   ///< Lower right trigger: RT, R2, ZR.
    Start,          ///< Right center menu button: Menu, Options, +, Start.
    LeftStick,      ///< Left analog thumbstick.
    RightStick,     ///< Right analog thumbstick.
    DpadVertical,   ///< Up and down on the directional pad.
    DpadHorizontal, ///< Left and right on the directional pad.
    Dpad,           ///< All four directions on the directional pad.
};

/// Classifies a controller from the raw device string reported by `GetGamepadName` (desktop HID or
/// browser Gamepad API). Unknown or empty strings fall back to `GamepadType::Generic`.
[[nodiscard]] GamepadType ClassifyGamepadName(std::string_view rawName);

/// Human-readable name of `type` (for logs, debug overlay and tests).
[[nodiscard]] const char* GamepadTypeName(GamepadType type);

/// Short text label printed on or beside `control` for `type` (for example `"RT"`, `"R2"` or `"ZR"`,
/// and `"A"` vs `"B"` on Switch where the physical bottom and right letters are swapped).
[[nodiscard]] const char* GamepadControlLabel(GamepadType type, GamepadControl control);

} // namespace horde
