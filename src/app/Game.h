#pragma once

#include "core/FixedTimestep.h"
#include "core/Session.h"

namespace horde {

/// Owns the window and runs the main loop: fixed-rate updates, then one render per frame.
class Game {
public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /// Runs until the player closes the window or presses Esc.
    void Run();

private:
    void Update(float dt);
    void Draw(float alpha) const;

    FixedTimestep _timestep;
    Session _session;

#ifndef NDEBUG
    bool _showDebugOverlay = false;
    Vec2 _lastMoveInput; ///< Kept only so the overlay can show what the last step read.
#endif
};

} // namespace horde
