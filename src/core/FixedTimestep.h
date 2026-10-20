#pragma once

namespace horde {

/// Turns variable frame times into a whole number of fixed simulation steps.
///
/// Gameplay runs at a fixed rate so movement, collisions and timers behave the same
/// on a 60 Hz laptop and on a 240 Hz monitor. Rendering still happens once per frame.
class FixedTimestep {
public:
    /// @param stepSeconds Length of one simulation step. Must be greater than zero.
    /// @param maxFrameSeconds Longest frame time accepted. A longer frame (a breakpoint, a window drag)
    ///        is clamped, so the game slows down for a moment instead of running hundreds of catch-up steps.
    explicit FixedTimestep(double stepSeconds = 1.0 / 60.0, double maxFrameSeconds = 0.25);

    /// Adds one frame of real time and returns how many fixed steps to simulate now.
    int Advance(double frameSeconds);

    /// How far the simulation is into the next step, from 0 (just stepped) to almost 1.
    /// The renderer uses it to interpolate positions between two steps.
    [[nodiscard]] double Alpha() const;

    [[nodiscard]] double StepSeconds() const { return _stepSeconds; }

private:
    double _stepSeconds;
    double _maxFrameSeconds;
    double _accumulator = 0.0;
};

} // namespace horde
