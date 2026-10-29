#include <doctest/doctest.h>

#include <ostream>
#include <string_view>

#include "core/GamepadType.h"

using horde::GamepadControl;
using horde::GamepadType;

TEST_CASE("gamepad names classify cleanly into Xbox, PlayStation, Switch and Generic") {
    SUBCASE("Xbox family names") {
        CHECK(horde::ClassifyGamepadName("Xbox Controller") == GamepadType::Xbox);
        CHECK(horde::ClassifyGamepadName("Wireless Xbox Controller") == GamepadType::Xbox);
        CHECK(horde::ClassifyGamepadName("Xbox Wireless Controller") == GamepadType::Xbox);
        CHECK(horde::ClassifyGamepadName("Controller (Xbox One For Windows)") == GamepadType::Xbox);
        CHECK(horde::ClassifyGamepadName("Microsoft X-Box 360 pad") == GamepadType::Xbox);
        CHECK(horde::ClassifyGamepadName("XInput Controller #1") == GamepadType::Xbox);
        CHECK(horde::ClassifyGamepadName("045e-02ea-Controller") == GamepadType::Xbox);
    }

    SUBCASE("PlayStation family names, including Windows DirectInput Wireless Controller") {
        CHECK(horde::ClassifyGamepadName("PS4 Controller") == GamepadType::PlayStation);
        CHECK(horde::ClassifyGamepadName("PS5 Controller") == GamepadType::PlayStation);
        CHECK(horde::ClassifyGamepadName("Afterglow PS3 Controller") == GamepadType::PlayStation);
        CHECK(horde::ClassifyGamepadName("Sony Interactive Entertainment DualSense Wireless Controller") ==
              GamepadType::PlayStation);
        CHECK(horde::ClassifyGamepadName("DualShock 4") == GamepadType::PlayStation);
        CHECK(horde::ClassifyGamepadName("  Wireless Controller  ") == GamepadType::PlayStation);
        CHECK(horde::ClassifyGamepadName("Wireless Controller (STANDARD GAMEPAD Vendor: 054c Product: 09cc)") ==
              GamepadType::PlayStation);
    }

    SUBCASE("Nintendo Switch family names") {
        CHECK(horde::ClassifyGamepadName("Nintendo Switch Pro Controller") == GamepadType::Switch);
        CHECK(horde::ClassifyGamepadName("Pro Controller") == GamepadType::Switch);
        CHECK(horde::ClassifyGamepadName("Pro Controller (STANDARD GAMEPAD Vendor: 057e Product: 2009)") ==
              GamepadType::Switch);
        CHECK(horde::ClassifyGamepadName("Joy-Con (L/R)") == GamepadType::Switch);
    }

    SUBCASE("unrecognized or empty names fall back to Generic") {
        CHECK(horde::ClassifyGamepadName("") == GamepadType::Generic);
        CHECK(horde::ClassifyGamepadName(" \t\r\n ") == GamepadType::Generic);
        CHECK(horde::ClassifyGamepadName("USB Gamepad") == GamepadType::Generic);
        CHECK(horde::ClassifyGamepadName("Logitech Dual Action") == GamepadType::Generic);
        CHECK(horde::ClassifyGamepadName("8BitDo SN30 Pro") == GamepadType::Generic);
        CHECK(horde::ClassifyGamepadName("Wireless Gamepad") == GamepadType::Generic);
    }

    SUBCASE("type names are human-readable") {
        CHECK(std::string_view(horde::GamepadTypeName(GamepadType::Xbox)) == "Xbox");
        CHECK(std::string_view(horde::GamepadTypeName(GamepadType::PlayStation)) == "PlayStation");
        CHECK(std::string_view(horde::GamepadTypeName(GamepadType::Switch)) == "Switch");
        CHECK(std::string_view(horde::GamepadTypeName(GamepadType::Generic)) == "Generic");
    }
}

TEST_CASE("gamepad control labels reflect positional buttons across all four families") {
    // Bottom face button (confirm): A on Xbox/Generic, Cross on PlayStation, B on Switch.
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::FaceSouth)) == "A");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Generic, GamepadControl::FaceSouth)) == "A");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::FaceSouth)) ==
          "Cross");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::FaceSouth)) == "B");

    // Right face button (back): B on Xbox/Generic, Circle on PlayStation, A on Switch.
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::FaceEast)) == "B");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::FaceEast)) ==
          "Circle");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::FaceEast)) == "A");

    // West (heal) and North (swap gun) swap X/Y on Switch and use Square/Triangle on PlayStation.
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::FaceWest)) == "X");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::FaceWest)) == "Y");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::FaceWest)) ==
          "Square");

    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::FaceNorth)) == "Y");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::FaceNorth)) == "X");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::FaceNorth)) ==
          "Triangle");

    // Triggers, bumpers and center menu buttons match each platform's naming.
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::LeftTrigger)) == "LT");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::LeftTrigger)) ==
          "L2");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::LeftTrigger)) == "ZL");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::RightTrigger)) == "RT");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::RightTrigger)) ==
          "R2");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::RightTrigger)) == "ZR");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Generic, GamepadControl::LeftBumper)) == "LB");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::LeftBumper)) ==
          "L1");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::LeftBumper)) == "L");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Generic, GamepadControl::RightBumper)) == "RB");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::RightBumper)) ==
          "R1");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::RightBumper)) == "R");

    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::Start)) == "Menu");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::PlayStation, GamepadControl::Start)) ==
          "Options");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Switch, GamepadControl::Start)) == "+");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Generic, GamepadControl::Start)) == "Start");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::LeftStick)) == "L");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::RightStick)) == "R");
    CHECK(std::string_view(horde::GamepadControlLabel(GamepadType::Xbox, GamepadControl::Dpad)) == "D-pad");
}
