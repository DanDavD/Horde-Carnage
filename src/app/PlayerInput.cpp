#include "app/PlayerInput.h"

#include <cstddef>

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "core/AnalogInput.h"

namespace horde {

namespace {

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

bool IsSlotAvailable(std::optional<int> gamepad) {
    return gamepad.has_value() && *gamepad >= 0 && *gamepad < kMaxGamepads && IsGamepadAvailable(*gamepad);
}

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

float ButtonAxis(int slot, int negativeButton, int positiveButton) {
    float axis = 0.0f;
    if (IsGamepadButtonDown(slot, negativeButton)) {
        axis -= 1.0f;
    }
    if (IsGamepadButtonDown(slot, positiveButton)) {
        axis += 1.0f;
    }
    return axis;
}

Vec2 ReadPadMoveSlot(int slot) {
    if (!IsGamepadAvailable(slot)) {
        return {};
    }
    const Vec2 raw{GetGamepadAxisMovement(slot, GAMEPAD_AXIS_LEFT_X),
                   GetGamepadAxisMovement(slot, GAMEPAD_AXIS_LEFT_Y)};
    const Vec2 stick = ApplyRadialDeadzone(raw, kStickDeadzone);
    // Some players move with the d-pad, as in older arcade brawlers. It works like the keyboard.
    const Vec2 dpad{ButtonAxis(slot, GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT),
                    ButtonAxis(slot, GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN)};
    return stick + dpad;
}

Vec2 ReadLookStickSlot(int slot) {
    if (!IsGamepadAvailable(slot)) {
        return {};
    }
    const Vec2 raw{GetGamepadAxisMovement(slot, GAMEPAD_AXIS_RIGHT_X),
                   GetGamepadAxisMovement(slot, GAMEPAD_AXIS_RIGHT_Y)};
    return ApplyRadialDeadzone(raw, kStickDeadzone);
}

bool HasSlotActivity(int slot) {
    if (!IsGamepadAvailable(slot)) {
        return false;
    }
    if (LengthSquared(ReadPadMoveSlot(slot)) > 0.0f || LengthSquared(ReadLookStickSlot(slot)) > 0.0f) {
        return true;
    }
    if (IsGamepadButtonDown(slot, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
        IsGamepadButtonDown(slot, GAMEPAD_BUTTON_LEFT_TRIGGER_2) ||
        GetGamepadAxisMovement(slot, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.15f ||
        GetGamepadAxisMovement(slot, GAMEPAD_AXIS_LEFT_TRIGGER) > 0.15f) {
        return true;
    }
    constexpr int kCheckedButtons[] = {
        GAMEPAD_BUTTON_LEFT_FACE_UP,
        GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
        GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        GAMEPAD_BUTTON_LEFT_FACE_LEFT,
        GAMEPAD_BUTTON_RIGHT_FACE_UP,
        GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
        GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
        GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
        GAMEPAD_BUTTON_LEFT_TRIGGER_1,
        GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
        GAMEPAD_BUTTON_MIDDLE_LEFT,
        GAMEPAD_BUTTON_MIDDLE,
        GAMEPAD_BUTTON_MIDDLE_RIGHT,
        GAMEPAD_BUTTON_LEFT_THUMB,
        GAMEPAD_BUTTON_RIGHT_THUMB,
    };
    for (const int button : kCheckedButtons) {
        if (IsGamepadButtonDown(slot, button) || IsGamepadButtonPressed(slot, button)) {
            return true;
        }
    }
    return false;
}

Vec2 ReadKeyMove() {
    return Vec2{KeyAxis(KEY_A, KEY_LEFT, KEY_D, KEY_RIGHT), KeyAxis(KEY_W, KEY_UP, KEY_S, KEY_DOWN)};
}

} // namespace

std::array<bool, kMaxGamepads> ReadGamepadPresence() {
    std::array<bool, kMaxGamepads> present{};
    for (int i = 0; i < kMaxGamepads; ++i) {
        present[static_cast<std::size_t>(i)] = IsGamepadAvailable(i);
    }
    return present;
}

std::optional<int> FindActiveGamepadSlot(std::optional<int> preferredSlot) {
    if (IsSlotAvailable(preferredSlot) && HasSlotActivity(*preferredSlot)) {
        return preferredSlot;
    }
    for (int i = 0; i < kMaxGamepads; ++i) {
        if (HasSlotActivity(i)) {
            return i;
        }
    }
    return std::nullopt;
}

const char* ReadGamepadName(std::optional<int> gamepad) {
    if (!IsSlotAvailable(gamepad)) {
        return "";
    }
    const char* raw = GetGamepadName(*gamepad);
    return raw != nullptr ? raw : "";
}

GamepadType ReadGamepadType(std::optional<int> gamepad) {
    return ClassifyGamepadName(ReadGamepadName(gamepad));
}

Vec2 ReadMoveInput(std::optional<int> gamepad) {
    // All devices add up. StepMovement clamps the sum to length 1, so using two at once is never faster.
    const Vec2 pad = IsSlotAvailable(gamepad) ? ReadPadMoveSlot(*gamepad) : Vec2{};
    return ReadKeyMove() + pad;
}

Vec2 ReadPadMenuDirection(std::optional<int> gamepad) {
    return IsSlotAvailable(gamepad) ? ReadPadMoveSlot(*gamepad) : Vec2{};
}

Vec2 ReadLookStick(std::optional<int> gamepad) {
    return IsSlotAvailable(gamepad) ? ReadLookStickSlot(*gamepad) : Vec2{};
}

std::optional<InputDevice> DetectActiveInputDevice(Vec2 currentMousePos, Vec2 lastMousePos,
                                                   std::optional<int> activeGamepadInputSlot) {
    if (activeGamepadInputSlot.has_value()) {
        return InputDevice::Gamepad;
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
        KEY_SPACE, KEY_LEFT_SHIFT, KEY_E,      KEY_Q,        KEY_ONE,       KEY_TWO,
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

bool ReadFireHeld(std::optional<int> gamepad) {
    const bool mouseFire = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool padFire = IsSlotAvailable(gamepad) &&
                         (IsGamepadButtonDown(*gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
                          GetGamepadAxisMovement(*gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.15f);
    return mouseFire || padFire;
}

void CollectButtonPresses(ButtonPresses& presses, std::optional<int> gamepad) {
    const bool pad = IsSlotAvailable(gamepad);
    const auto padPressed = [pad, gamepad](int button) {
        return pad && IsGamepadButtonPressed(*gamepad, button);
    };

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

MenuInput ReadMenuInput(MenuRepeat& padRepeat, float dt, std::optional<int> gamepad) {
    const MenuRepeatStep padStep = StepMenuRepeat(padRepeat, ReadPadMenuDirection(gamepad), dt);
    const bool pad = IsSlotAvailable(gamepad);
    const auto padPressed = [pad, gamepad](int button) {
        return pad && IsGamepadButtonPressed(*gamepad, button);
    };

    MenuInput input;
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        input.move = -1;
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        input.move = 1;
    }
    // Repeated while held, so the volume can be run from one end to the other without tapping.
    if (IsKeyPressed(KEY_A) || IsKeyPressedRepeat(KEY_A) || IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        input.adjust = -1;
    }
    if (IsKeyPressed(KEY_D) || IsKeyPressedRepeat(KEY_D) || IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        input.adjust = 1;
    }
    if (input.move == 0) {
        input.move = padStep.move;
    }
    if (input.adjust == 0) {
        input.adjust = padStep.adjust;
    }

    input.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
                    padPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    input.back = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || padPressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    return input;
}

bool ReadPausePressed(std::optional<int> gamepad) {
    return IsKeyPressed(KEY_ESCAPE) ||
           (IsSlotAvailable(gamepad) && IsGamepadButtonPressed(*gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT));
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
