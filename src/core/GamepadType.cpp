#include "core/GamepadType.h"

#include <cstddef>

namespace horde {

namespace {

char AsciiLower(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

bool IsAsciiSpace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

std::string_view TrimAsciiWhitespace(std::string_view text) {
    while (!text.empty() && IsAsciiSpace(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && IsAsciiSpace(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty() || haystack.size() < needle.size()) {
        return false;
    }
    const std::size_t limit = haystack.size() - needle.size();
    for (std::size_t i = 0; i <= limit; ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            if (AsciiLower(haystack[i + j]) != AsciiLower(needle[j])) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

bool EqualsCaseInsensitive(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (AsciiLower(a[i]) != AsciiLower(b[i])) {
            return false;
        }
    }
    return true;
}

bool StartsWithCaseInsensitive(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && EqualsCaseInsensitive(text.substr(0, prefix.size()), prefix);
}

} // namespace

GamepadType ClassifyGamepadName(std::string_view rawName) {
    const std::string_view name = TrimAsciiWhitespace(rawName);
    if (name.empty()) {
        return GamepadType::Generic;
    }

    // Explicit brand and USB vendor tokens first, so "Xbox Wireless Controller" never falls into Sony's
    // generic HID name "Wireless Controller" below.
    constexpr std::string_view kPlayStationTokens[] = {
        "playstation", "dualshock", "dualsense", "sixaxis", "sony", "ps5", "ps4", "ps3", "ps2", "054c",
    };
    for (const std::string_view token : kPlayStationTokens) {
        if (ContainsCaseInsensitive(name, token)) {
            return GamepadType::PlayStation;
        }
    }

    constexpr std::string_view kSwitchTokens[] = {
        "nintendo", "switch", "joy-con", "joycon", "057e", "horipad s",
    };
    for (const std::string_view token : kSwitchTokens) {
        if (ContainsCaseInsensitive(name, token)) {
            return GamepadType::Switch;
        }
    }

    constexpr std::string_view kXboxTokens[] = {
        "xbox", "x-box", "xinput", "microsoft", "045e", "360 controller",
    };
    for (const std::string_view token : kXboxTokens) {
        if (ContainsCaseInsensitive(name, token)) {
            return GamepadType::Xbox;
        }
    }

    // Official DualShock 4 over Windows DirectInput reports literally "Wireless Controller".
    if (EqualsCaseInsensitive(name, "wireless controller") ||
        StartsWithCaseInsensitive(name, "wireless controller (")) {
        return GamepadType::PlayStation;
    }

    // Official Switch Pro Controller over Bluetooth/USB on Windows reports "Pro Controller".
    if (ContainsCaseInsensitive(name, "pro controller")) {
        return GamepadType::Switch;
    }

    return GamepadType::Generic;
}

const char* GamepadTypeName(GamepadType type) {
    switch (type) {
    case GamepadType::Xbox:
        return "Xbox";
    case GamepadType::PlayStation:
        return "PlayStation";
    case GamepadType::Switch:
        return "Switch";
    case GamepadType::Generic:
        return "Generic";
    }
    return "Generic";
}

const char* GamepadControlLabel(GamepadType type, GamepadControl control) {
    switch (control) {
    case GamepadControl::FaceSouth:
        if (type == GamepadType::PlayStation) return "Cross";
        if (type == GamepadType::Switch) return "B";
        return "A";
    case GamepadControl::FaceEast:
        if (type == GamepadType::PlayStation) return "Circle";
        if (type == GamepadType::Switch) return "A";
        return "B";
    case GamepadControl::FaceWest:
        if (type == GamepadType::PlayStation) return "Square";
        if (type == GamepadType::Switch) return "Y";
        return "X";
    case GamepadControl::FaceNorth:
        if (type == GamepadType::PlayStation) return "Triangle";
        if (type == GamepadType::Switch) return "X";
        return "Y";
    case GamepadControl::LeftBumper:
        if (type == GamepadType::PlayStation) return "L1";
        if (type == GamepadType::Switch) return "L";
        return "LB";
    case GamepadControl::RightBumper:
        if (type == GamepadType::PlayStation) return "R1";
        if (type == GamepadType::Switch) return "R";
        return "RB";
    case GamepadControl::LeftTrigger:
        if (type == GamepadType::PlayStation) return "L2";
        if (type == GamepadType::Switch) return "ZL";
        return "LT";
    case GamepadControl::RightTrigger:
        if (type == GamepadType::PlayStation) return "R2";
        if (type == GamepadType::Switch) return "ZR";
        return "RT";
    case GamepadControl::Start:
        if (type == GamepadType::Xbox) return "Menu";
        if (type == GamepadType::PlayStation) return "Options";
        if (type == GamepadType::Switch) return "+";
        return "Start";
    case GamepadControl::LeftStick:
        return "L";
    case GamepadControl::RightStick:
        return "R";
    case GamepadControl::DpadVertical:
    case GamepadControl::DpadHorizontal:
    case GamepadControl::Dpad:
        return "D-pad";
    }
    return "";
}

const char* GamepadActionBinding(GamepadType type, GamepadAction action) {
    switch (action) {
    case GamepadAction::Move:
        return "Left stick or d-pad";
    case GamepadAction::Aim:
        return "Right stick";
    case GamepadAction::Fire:
        if (type == GamepadType::PlayStation) return "R2, hold with the SMG";
        if (type == GamepadType::Switch) return "ZR, hold with the SMG";
        return "RT, hold with the SMG";
    case GamepadAction::SwitchGun:
        if (type == GamepadType::PlayStation) return "Triangle";
        if (type == GamepadType::Switch) return "X";
        return "Y";
    case GamepadAction::Dash:
        if (type == GamepadType::PlayStation) return "L2 or R1";
        if (type == GamepadType::Switch) return "ZL or R";
        return "LT or RB";
    case GamepadAction::Flask:
        if (type == GamepadType::PlayStation) return "Square";
        if (type == GamepadType::Switch) return "Y";
        return "X";
    case GamepadAction::Pause:
        if (type == GamepadType::Xbox) return "Menu";
        if (type == GamepadType::PlayStation) return "Options";
        if (type == GamepadType::Switch) return "+";
        return "Start";
    case GamepadAction::Retry:
        if (type == GamepadType::PlayStation) return "- (Cross on end screen)";
        if (type == GamepadType::Switch) return "- (B on end screen)";
        return "- (A on end screen)";
    }
    return "-";
}

const char* GamepadActionBindingSummary(GamepadAction action) {
    switch (action) {
    case GamepadAction::Move:
        return "Left stick or d-pad";
    case GamepadAction::Aim:
        return "Right stick";
    case GamepadAction::Fire:
        return "RT / R2 / ZR, hold with SMG";
    case GamepadAction::SwitchGun:
        return "Y / Triangle";
    case GamepadAction::Dash:
        return "LT or RB (L2 / R1)";
    case GamepadAction::Flask:
        return "X / Square";
    case GamepadAction::Pause:
        return "Start / Options / Menu";
    case GamepadAction::Retry:
        return "- (A on end screen)";
    }
    return "-";
}

} // namespace horde
