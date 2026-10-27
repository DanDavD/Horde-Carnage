#pragma once

#include "app/PlayerInput.h"
#include "app/render/Art.h"
#include "core/FixedTimestep.h"
#include "core/HitStop.h"
#include "core/ScreenShake.h"
#include "core/Session.h"

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
    /// One frame: the presses since the last one, the fixed steps that are due, and one render.
    void Frame();
    void Update(float dt);
    /// Turns what happened in the last step into a hit stop and a screen shake, and clears the events.
    void FeelEvents();
    void Draw(float alpha) const;

    FixedTimestep _timestep;
    Session _session;
    ButtonPresses _pendingPresses; ///< Presses since the last fixed step, used by the next one.
    Art _art; ///< Loaded once the window is open, unloaded before it closes.
    // Both live here, not in the World: they change how the fight is shown and when it steps, never what
    // happens in it.
    HitStop _hitStop;
    ScreenShake _shake;

#ifndef NDEBUG
    bool _showDebugOverlay = false;
    Vec2 _lastMoveInput; ///< Kept only so the overlay can show what the last step read.
#endif
};

} // namespace horde
