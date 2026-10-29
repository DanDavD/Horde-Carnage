#include "app/PlayerInput.h"

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "core/AnalogInput.h"

namespace horde {

namespace {

constexpr int kGamepad = 0;

#ifdef __EMSCRIPTEN__
// The page locks the pointer on the next click only while the game wants it hidden (see web/shell.html).
EM_JS(void, SetPageWantsLock, (int wants), {
    Module.hordeWantsLock = wants != 0;
    if (!wants && document.pointerLockElement) {
        document.exitPointerLock();
    }
});

EM_JS(int, TakePageLockLost, (), {
    const lost = Module.hordeLockLost ? 1 : 0;
    Module.hordeLockLost = false;
    return lost;
});
#endif
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

Vec2 ReadPadMove() {
    if (!IsGamepadAvailable(kGamepad)) {
        return {};
    }
    const Vec2 raw{GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_X),
                   GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_Y)};
    const Vec2 stick = ApplyRadialDeadzone(raw, kStickDeadzone);
    // Some players move with the d-pad, as in older arcade brawlers. It works like the keyboard.
    const Vec2 dpad{ButtonAxis(GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT),
                    ButtonAxis(GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN)};
    return stick + dpad;
}

Vec2 ReadKeyMove() {
    return Vec2{KeyAxis(KEY_A, KEY_LEFT, KEY_D, KEY_RIGHT), KeyAxis(KEY_W, KEY_UP, KEY_S, KEY_DOWN)};
}

} // namespace

Vec2 ReadMoveInput() {
    // All devices add up. StepMovement clamps the sum to length 1, so using two at once is never faster.
    return ReadKeyMove() + ReadPadMove();
}

Vec2 ReadLookStick() {
    if (!IsGamepadAvailable(kGamepad)) {
        return {};
    }
    const Vec2 raw{GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_RIGHT_X),
                   GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_RIGHT_Y)};
    return ApplyRadialDeadzone(raw, kStickDeadzone);
}

std::optional<InputDevice> DetectActiveInputDevice(Vec2 currentMousePos, Vec2 lastMousePos) {
    if (IsGamepadAvailable(kGamepad)) {
        if (LengthSquared(ReadPadMove()) > 0.0f || LengthSquared(ReadLookStick()) > 0.0f) {
            return InputDevice::Gamepad;
        }
        if (GetGamepadButtonPressed() != GAMEPAD_BUTTON_UNKNOWN) {
            return InputDevice::Gamepad;
        }
        if (IsGamepadButtonDown(kGamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
            IsGamepadButtonDown(kGamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_2) ||
            GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.15f ||
            GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_TRIGGER) > 0.15f) {
            return InputDevice::Gamepad;
        }
        constexpr int kCheckedButtons[] = {
            GAMEPAD_BUTTON_RIGHT_FACE_UP,
            GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
            GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
            GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
            GAMEPAD_BUTTON_LEFT_TRIGGER_1,
            GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
            GAMEPAD_BUTTON_MIDDLE_RIGHT,
        };
        for (const int button : kCheckedButtons) {
            if (IsGamepadButtonDown(kGamepad, button)) {
                return InputDevice::Gamepad;
            }
        }
    }

    if (LengthSquared(currentMousePos - lastMousePos) > 4.0f) {
        return InputDevice::KeyboardMouse;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
        IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || GetMouseWheelMove() != 0.0f) {
        return InputDevice::KeyboardMouse;
    }
    if (LengthSquared(ReadKeyMove()) > 0.0f) {
        return InputDevice::KeyboardMouse;
    }
    constexpr int kCheckedKeys[] = {
        KEY_SPACE, KEY_LEFT_SHIFT, KEY_E,      KEY_Q,     KEY_ONE,      KEY_TWO,
        KEY_ESCAPE, KEY_ENTER,     KEY_KP_ENTER, KEY_BACKSPACE, KEY_R,
    };
    for (const int key : kCheckedKeys) {
        if (IsKeyDown(key) || IsKeyPressed(key)) {
            return InputDevice::KeyboardMouse;
        }
    }

    return std::nullopt;
}

Vec2 ReadAimPoint() {
#ifdef __EMSCRIPTEN__
    // The page keeps the aim point up to date in Module.hordeAim, already in game pixels.
    const double x = EM_ASM_DOUBLE({ return Module.hordeAim ? Module.hordeAim.x : 640; });
    const double y = EM_ASM_DOUBLE({ return Module.hordeAim ? Module.hordeAim.y : 360; });
    return Vec2{static_cast<float>(x), static_cast<float>(y)};
#else
    const Vector2 mouse = GetMousePosition();
    return Vec2{mouse.x, mouse.y};
#endif
}

void SetCursorHidden(bool hidden) {
#ifdef __EMSCRIPTEN__
    SetPageWantsLock(hidden ? 1 : 0);
#else
    if (hidden && !IsCursorHidden()) {
        HideCursor();
    } else if (!hidden && IsCursorHidden()) {
        ShowCursor();
    }
#endif
}

bool ReadFireHeld() {
    const bool mouseFire = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool padFire = IsGamepadAvailable(kGamepad) &&
                         (IsGamepadButtonDown(kGamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
                          GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.15f);
    return mouseFire || padFire;
}

void CollectButtonPresses(ButtonPresses& presses) {
    const bool pad = IsGamepadAvailable(kGamepad);
    const auto padPressed = [pad](int button) { return pad && IsGamepadButtonPressed(kGamepad, button); };

    // Fire: left mouse button or RT.
    presses.fire = presses.fire || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                   padPressed(GAMEPAD_BUTTON_RIGHT_TRIGGER_2);

    if (IsKeyPressed(KEY_ONE)) {
        presses.weaponSlot = 0;
    }
    if (IsKeyPressed(KEY_TWO)) {
        presses.weaponSlot = 1;
    }

    // With two guns, either wheel direction or Y / Triangle means "the other gun".
    presses.cycleWeapon = presses.cycleWeapon || IsKeyPressed(KEY_Q) || GetMouseWheelMove() != 0.0f ||
                          padPressed(GAMEPAD_BUTTON_RIGHT_FACE_UP);

    // Dash: Space, left Shift, LT or RB.
    presses.dash = presses.dash || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT) ||
                   padPressed(GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || padPressed(GAMEPAD_BUTTON_LEFT_TRIGGER_2);

    // Heal: E, or X / Square.
    presses.heal = presses.heal || IsKeyPressed(KEY_E) || padPressed(GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
}

MenuInput ReadMenuKeys() {
    const bool pad = IsGamepadAvailable(kGamepad);
    const auto padPressed = [pad](int button) { return pad && IsGamepadButtonPressed(kGamepad, button); };
    MenuInput input;
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || padPressed(GAMEPAD_BUTTON_LEFT_FACE_UP)) {
        input.move = -1;
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN) || padPressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
        input.move = 1;
    }
    // Repeated while held, so the volume can be run from one end to the other without tapping.
    if (IsKeyPressed(KEY_A) || IsKeyPressedRepeat(KEY_A) || IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT) ||
        padPressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
        input.adjust = -1;
    }
    if (IsKeyPressed(KEY_D) || IsKeyPressedRepeat(KEY_D) || IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT) ||
        padPressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
        input.adjust = 1;
    }
    input.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
                    padPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    input.back = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || padPressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    return input;
}

bool ReadPausePressed() {
    return IsKeyPressed(KEY_ESCAPE) ||
           (IsGamepadAvailable(kGamepad) && IsGamepadButtonPressed(kGamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT));
}

bool ReadQuickRetryPressed() {
    return IsKeyPressed(KEY_R);
}

bool TakePointerLockLost() {
#ifdef __EMSCRIPTEN__
    return TakePageLockLost() != 0;
#else
    return false;
#endif
}

} // namespace horde
