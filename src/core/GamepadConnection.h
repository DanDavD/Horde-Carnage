#pragma once

#include <array>
#include <optional>

namespace horde {

/// Number of gamepad slots polled from raylib (`MAX_GAMEPADS`).
inline constexpr int kMaxGamepads = 4;

/// Tracks which gamepad slots are plugged in across frames and which slot drives gamepad input.
struct GamepadConnection {
    std::array<bool, kMaxGamepads> present{};
    std::optional<int> activeSlot;
    bool initialized = false;
};

/// Transitions detected on the gamepad slots during a single frame.
struct GamepadConnectionEvents {
    bool connected = false;          ///< A gamepad was plugged in after start-up.
    bool activeDisconnected = false; ///< The active gamepad unplugged this frame.
};

/// Updates `connection` from the slots currently reported by the platform and the slot (if any) that
/// produced stick or button input this frame.
///
/// On the first call (`!connection.initialized`), records whatever is already plugged in at launch without
/// reporting a hotplug event. On later frames, plugging in a pad selects it as `activeSlot`, using another
/// connected pad switches `activeSlot` to it, and unplugging `activeSlot` sets `activeDisconnected` so an
/// ongoing fight can pause.
[[nodiscard]] GamepadConnectionEvents StepGamepadConnection(GamepadConnection& connection,
                                                            std::array<bool, kMaxGamepads> nowPresent,
                                                            std::optional<int> activeInputSlot = std::nullopt);

} // namespace horde
