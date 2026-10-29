#pragma once

#include <optional>

#include "app/PlayerInput.h"
#include "app/audio/SoundBank.h"
#include "app/render/Art.h"
#include "app/render/Backdrop.h"
#include "app/render/Hud.h"
#include "app/render/MenuDraw.h"
#include "core/FixedTimestep.h"
#include "core/GamepadAim.h"
#include "core/GamepadConnection.h"
#include "core/HitStop.h"
#include "core/Menu.h"
#include "core/MenuRepeat.h"
#include "core/ParticleEmitter.h"
#include "core/ScreenFit.h"
#include "core/ScreenShake.h"
#include "core/Session.h"
#include "core/Settings.h"

namespace horde {

/// Owns the window and runs the main loop: fixed-rate updates, then one render per frame.
class Game {
public:
    /// Opens the game fullscreen at the monitor's resolution, or in a window that can be resized when
    /// `windowed` is set (`--windowed` on the command line). In a browser the page decides, and this is
    /// ignored.
    explicit Game(bool windowed = false);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /// Runs until the player closes the window or presses Esc. In a browser the page calls one frame at a
    /// time instead, and this never returns.
    void Run();

private:
    /// One frame: the menus, the presses since the last one, the fixed steps that are due, and one render.
    void Frame();
    /// Reads the menu on the screen, if any, and acts on a choice. While playing, pauses on Esc, Start or
    /// an unplugged gamepad.
    void HandleMenus(float frameSeconds, bool padDisconnected);
    /// The page on the screen: one opened over the title or the pause, or else the one of the session's state.
    [[nodiscard]] MenuPage CurrentPage() const;
    /// Shows `page` with `selected` selected, over the session's own page or as it.
    void OpenPage(MenuPage page, int selected);
    /// Does what the chosen item of `page` says.
    void Choose(MenuPage page, int item);
    /// Leaves `page` for the one it was opened from. The pause resumes the fight.
    void GoBack(MenuPage page);
    /// Changes one setting by `step`, puts it into effect and saves it.
    void ChangeSetting(Setting setting, int step);
    /// Puts the volume and the frame rate of `_settings` into effect.
    void ApplySettings();
    /// Makes the window fullscreen or not, as `_settings` says.
    void ApplyDisplay();
    /// Clears what belongs to the last fight: particles, the trails behind the bars, the slow-down and the
    /// shake, and presses made in a menu.
    void ForgetLastFight();
    /// Draws the world and, if asked, the HUD, for the screen fit `fit`. `alpha` blends the last two steps.
    void DrawScene(const ScreenFit& fit, float alpha, bool withHud) const;
    void Update(float dt);
    /// Turns what happened in the last step into hit stops, screen shakes and particles, and clears the events.
    void FeelEvents();
    /// Moves the trails behind the health bars on by one step, after the world has stepped.
    void StepHealthTrails(float dt);
    /// Trails with nothing behind either bar, for the start of a fight.
    void ResetHealthTrails();
    void Draw(float alpha) const;
    /// The settings or the controls, opened over the title or the pause.
    void DrawOpenedPage() const;

    FixedTimestep _timestep;
    Session _session;
    ButtonPresses _pendingPresses; ///< Presses since the last fixed step, used by the next one.
    Art _art; ///< Loaded once the window is open, unloaded before it closes.
    // All three live here, not in the World: they change how the fight is shown and when it steps, never
    // what happens in it.
    HitStop _hitStop;
    ScreenShake _shake;
    ParticleEmitter _particles;
    SoundBank _sounds; ///< Loaded once the audio device is open, unloaded before it closes.
    Random _soundRandom; ///< For the sounds' small changes of pitch and volume, apart from the fight's.
    HealthTrails _trails; ///< Behind the health bars. Kept here because only the HUD reads them.
    Settings _settings;    ///< Loaded before the window opens, saved on every change.
    /// The saved display, while `--windowed` overrides it for this run, so changing another setting does not
    /// save the window too.
    std::optional<bool> _savedFullscreen;
    Menu _menu;            ///< The menu on the screen; rebuilt whenever the page changes.
    MenuPage _menuPage = MenuPage::None; ///< The page `_menu` was built for.
    MenuPage _openedPage = MenuPage::None; ///< The settings or the controls, over the title or the pause.
    float _menuSinceMoved = 0.0f; ///< Real seconds since the selection last moved, for the marks to slide.
    Vec2 _lastMouse;       ///< The mouse last frame: only a mouse that moves takes the selection.
    InputDevice _inputDevice = InputDevice::KeyboardMouse;
    GamepadConnection _gamepadConnection;
    MenuRepeat _menuRepeat;
    GamepadAim _gamepadAim;
    Backdrop _backdrop;    ///< The blurred still behind the pause menu.
    bool _quit = false;    ///< Set by the title's Quit.

#ifndef NDEBUG
    bool _showDebugOverlay = false;
    Vec2 _lastMoveInput; ///< Kept only so the overlay can show what the last step read.
#endif
};

} // namespace horde
