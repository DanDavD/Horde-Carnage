#pragma once

#include <optional>

#include "core/Menu.h"
#include "core/Vec2.h"

namespace horde {

/// Which device the player interacted with most recently.
enum class InputDevice {
    KeyboardMouse,
    Gamepad,
};

/// Reads where the player wants to move this step, from the keyboard (WASD or arrows) and the
/// left stick and d-pad of the first gamepad. The result can be longer than 1; `StepMovement` clamps it.
[[nodiscard]] Vec2 ReadMoveInput();

/// Reads the right stick of the first gamepad, with deadzone applied. Returns {0, 0} if untouched
/// or inside the deadzone.
[[nodiscard]] Vec2 ReadLookStick();

/// Checks whether any mouse/keyboard or gamepad activity occurred this frame, returning the active device,
/// or std::nullopt if the controls are idle.
[[nodiscard]] std::optional<InputDevice> DetectActiveInputDevice(Vec2 currentMousePos, Vec2 lastMousePos);

/// This frame's menu keys: W, S, up and down (or the d-pad) move, A, D, left and right (or the d-pad)
/// adjust, Enter or Space (or A) confirm, Esc or Backspace (or B) go back. Left and right repeat while held.
/// The mouse is left to the caller, which knows where the items are. Read once per frame.
[[nodiscard]] MenuInput ReadMenuKeys();

/// Whether the pause button went down this frame: Esc, or Start on the first gamepad.
[[nodiscard]] bool ReadPausePressed();

/// Whether R went down this frame: a retry without going through the menu, for the next attempt at once.
[[nodiscard]] bool ReadQuickRetryPressed();

/// In a browser, whether the page let go of the pointer while the game held it since the last call: Esc
/// there is taken by the browser to free the mouse and never reaches the game, so this is how it pauses.
/// Always false on the desktop.
[[nodiscard]] bool TakePointerLockLost();

/// Where the mouse points, in screen pixels (1280x720, whatever size the window is shown at).
///
/// On the desktop that is raylib's mouse position. In a browser, raylib 5.5 stops following the mouse
/// once the cursor is hidden, so the page (`web/shell.html`) tracks the aim point itself, and keeps
/// following it while the pointer is locked to the game.
[[nodiscard]] Vec2 ReadAimPoint();

/// Shows or hides the system cursor over the game. The game draws its own crosshair while playing. In a
/// browser the page hides the cursor itself, by locking the pointer on a click while this asks for it, and
/// lets it go at once when it does not, so the menus can be used with the mouse.
void SetCursorHidden(bool hidden);

/// Whether the fire button (left mouse button or gamepad RT) is down right now. A held state, so reading
/// it once per fixed step is right.
[[nodiscard]] bool ReadFireHeld();

/// Button presses that are events, gathered over one or more frames until a fixed step uses them.
struct ButtonPresses {
    bool fire = false;              ///< Left mouse button or gamepad RT.
    std::optional<int> weaponSlot;  ///< Keys 1 and 2 pick slots 0 and 1.
    bool cycleWeapon = false;       ///< Q, mouse wheel, or gamepad Y/Triangle.
    bool dash = false;              ///< Space, left Shift, gamepad LT or RB.
    bool heal = false;              ///< E, or gamepad X/Square.
};

/// Adds this frame's presses to `presses`. Call it once per frame: a frame that runs no fixed step keeps
/// them for the next one, instead of losing them.
void CollectButtonPresses(ButtonPresses& presses);

} // namespace horde
