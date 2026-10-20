#pragma once

#include "core/FixedTimestep.h"

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
    double _simulationTime = 0.0;
};

} // namespace horde
