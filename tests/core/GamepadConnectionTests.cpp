#include <doctest/doctest.h>

#include "core/GamepadConnection.h"

using horde::GamepadConnection;
using horde::GamepadConnectionEvents;

TEST_CASE("initial gamepad scan records connected pads without reporting hotplug events") {
    SUBCASE("no gamepad at launch") {
        GamepadConnection conn;
        const GamepadConnectionEvents events =
            horde::StepGamepadConnection(conn, {false, false, false, false});
        CHECK_FALSE(events.connected);
        CHECK_FALSE(events.activeDisconnected);
        CHECK_FALSE(conn.activeSlot.has_value());
    }

    SUBCASE("slot 0 already plugged in at launch") {
        GamepadConnection conn;
        const GamepadConnectionEvents events =
            horde::StepGamepadConnection(conn, {true, false, false, false});
        CHECK_FALSE(events.connected);
        CHECK_FALSE(events.activeDisconnected);
        REQUIRE(conn.activeSlot.has_value());
        CHECK(*conn.activeSlot == 0);
    }
}

TEST_CASE("plugging in and unplugging a gamepad mid-run reports connect and disconnect") {
    GamepadConnection conn;
    (void)horde::StepGamepadConnection(conn, {false, false, false, false});

    // Plugging into slot 0 mid-fight selects slot 0 without pausing.
    GamepadConnectionEvents events = horde::StepGamepadConnection(conn, {true, false, false, false});
    CHECK(events.connected);
    CHECK_FALSE(events.activeDisconnected);
    REQUIRE(conn.activeSlot.has_value());
    CHECK(*conn.activeSlot == 0);

    // Steady state reports no transitions.
    events = horde::StepGamepadConnection(conn, {true, false, false, false});
    CHECK_FALSE(events.connected);
    CHECK_FALSE(events.activeDisconnected);

    // Unplugging slot 0 reports activeDisconnected so an ongoing fight pauses.
    events = horde::StepGamepadConnection(conn, {false, false, false, false});
    CHECK_FALSE(events.connected);
    CHECK(events.activeDisconnected);
    CHECK_FALSE(conn.activeSlot.has_value());

    // Reconnecting on slot 1 (common with wireless dongles) picks up slot 1 automatically.
    events = horde::StepGamepadConnection(conn, {false, true, false, false});
    CHECK(events.connected);
    CHECK_FALSE(events.activeDisconnected);
    REQUIRE(conn.activeSlot.has_value());
    CHECK(*conn.activeSlot == 1);
}

TEST_CASE("input on another connected slot switches active slot, and only active slot unplug pauses") {
    GamepadConnection conn;
    (void)horde::StepGamepadConnection(conn, {true, true, false, false});
    REQUIRE(conn.activeSlot.has_value());
    CHECK(*conn.activeSlot == 0);

    // Player picks up controller 1 and presses a button.
    GamepadConnectionEvents events = horde::StepGamepadConnection(conn, {true, true, false, false}, 1);
    CHECK_FALSE(events.activeDisconnected);
    REQUIRE(conn.activeSlot.has_value());
    CHECK(*conn.activeSlot == 1);

    // Idle controller 0 disconnects: active slot 1 keeps going without a false disconnect pause.
    events = horde::StepGamepadConnection(conn, {false, true, false, false});
    CHECK_FALSE(events.activeDisconnected);
    REQUIRE(conn.activeSlot.has_value());
    CHECK(*conn.activeSlot == 1);

    // Reconnect slot 0, then unplug active slot 1: activeDisconnected fires and falls back to slot 0.
    (void)horde::StepGamepadConnection(conn, {true, true, false, false}, 1);
    events = horde::StepGamepadConnection(conn, {true, false, false, false});
    CHECK(events.activeDisconnected);
    REQUIRE(conn.activeSlot.has_value());
    CHECK(*conn.activeSlot == 0);
}
