#include "core/GamepadConnection.h"

#include <cstddef>

namespace horde {

namespace {

std::optional<int> FirstPresentSlot(const std::array<bool, kMaxGamepads>& present) {
    for (int i = 0; i < kMaxGamepads; ++i) {
        if (present[static_cast<std::size_t>(i)]) {
            return i;
        }
    }
    return std::nullopt;
}

bool IsValidPresentSlot(std::optional<int> slot, const std::array<bool, kMaxGamepads>& present) {
    return slot.has_value() && *slot >= 0 && *slot < kMaxGamepads &&
           present[static_cast<std::size_t>(*slot)];
}

} // namespace

GamepadConnectionEvents StepGamepadConnection(GamepadConnection& connection,
                                              std::array<bool, kMaxGamepads> nowPresent,
                                              std::optional<int> activeInputSlot) {
    if (!connection.initialized) {
        connection.initialized = true;
        connection.present = nowPresent;
        if (IsValidPresentSlot(activeInputSlot, nowPresent)) {
            connection.activeSlot = activeInputSlot;
        } else {
            connection.activeSlot = FirstPresentSlot(nowPresent);
        }
        return {};
    }

    GamepadConnectionEvents events;

    if (connection.activeSlot.has_value() &&
        !nowPresent[static_cast<std::size_t>(*connection.activeSlot)]) {
        events.activeDisconnected = true;
        connection.activeSlot.reset();
    }

    std::optional<int> newlyConnectedSlot;
    for (int i = 0; i < kMaxGamepads; ++i) {
        const auto idx = static_cast<std::size_t>(i);
        if (!connection.present[idx] && nowPresent[idx]) {
            events.connected = true;
            if (!newlyConnectedSlot.has_value()) {
                newlyConnectedSlot = i;
            }
        }
    }

    if (IsValidPresentSlot(activeInputSlot, nowPresent)) {
        connection.activeSlot = activeInputSlot;
    } else if (newlyConnectedSlot.has_value()) {
        connection.activeSlot = newlyConnectedSlot;
    } else if (!IsValidPresentSlot(connection.activeSlot, nowPresent)) {
        connection.activeSlot = FirstPresentSlot(nowPresent);
    }

    connection.present = nowPresent;
    return events;
}

} // namespace horde
