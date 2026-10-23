#include "app/PlayerInput.h"

#include <raylib.h>

#include "core/AnalogInput.h"

namespace horde {

namespace {

constexpr int kGamepad = 0;
constexpr float kStickDeadzone = 0.2f;

float KeyAxis(int negativeKey, int negativeAltKey, int positiveKey, int positiveAltKey) {
    float axis = 0.0f;
    if (IsKeyDown(negativeKey) || IsKeyDown(negativeAltKey)) {
        axis -= 1.0f;
    }
    if (IsKeyDown(positiveKey) || IsKeyDown(positiveAltKey)) {
        axis += 1.0f;
    }
    return axis;
}

float ButtonAxis(int negativeButton, int positiveButton) {
    float axis = 0.0f;
    if (IsGamepadButtonDown(kGamepad, negativeButton)) {
        axis -= 1.0f;
    }
    if (IsGamepadButtonDown(kGamepad, positiveButton)) {
        axis += 1.0f;
    }
    return axis;
}

} // namespace

Vec2 ReadMoveInput() {
    const Vec2 keys{KeyAxis(KEY_A, KEY_LEFT, KEY_D, KEY_RIGHT), KeyAxis(KEY_W, KEY_UP, KEY_S, KEY_DOWN)};

    Vec2 stick;
    Vec2 dpad;
    if (IsGamepadAvailable(kGamepad)) {
        const Vec2 raw{GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_X),
                       GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_Y)};
        stick = ApplyRadialDeadzone(raw, kStickDeadzone);
        // Some players move with the d-pad, as in older arcade brawlers. It works like the keyboard.
        dpad = {ButtonAxis(GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT),
                ButtonAxis(GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN)};
    }

    // All devices add up. StepMovement clamps the sum to length 1, so using two at once is never faster.
    return keys + stick + dpad;
}

bool ReadFireHeld() {
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

void CollectButtonPresses(ButtonPresses& presses) {
    presses.fire = presses.fire || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (IsKeyPressed(KEY_ONE)) {
        presses.weaponSlot = 0;
    }
    if (IsKeyPressed(KEY_TWO)) {
        presses.weaponSlot = 1;
    }
    // With two guns, either wheel direction means "the other gun".
    presses.cycleWeapon = presses.cycleWeapon || IsKeyPressed(KEY_Q) || GetMouseWheelMove() != 0.0f;
    presses.dash = presses.dash || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT);
    presses.heal = presses.heal || IsKeyPressed(KEY_E);
}

bool ReadRestartPressed() {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
        return true;
    }
    return IsGamepadAvailable(kGamepad) && (IsGamepadButtonPressed(kGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
                                           IsGamepadButtonPressed(kGamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT));
}

} // namespace horde
