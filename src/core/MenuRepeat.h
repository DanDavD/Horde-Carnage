#pragma once

#include "core/Vec2.h"

namespace horde {

/// Timing and stick thresholds for stepping through menus by holding a stick or d-pad.
struct MenuRepeatStats {
    float engageThreshold = 0.5f;   ///< Tilt needed to start moving or adjusting.
    float releaseThreshold = 0.35f; ///< Tilt below which the stick counts as let go again.
    float initialDelay = 0.38f;     ///< Pause after the first step before holding begins to repeat.
    float repeatInterval = 0.11f;   ///< Pause between repeated steps while held.
};

/// Discrete menu pulses produced in a single frame.
struct MenuRepeatStep {
    int move = 0;   ///< -1 up, 1 down, 0 none.
    int adjust = 0; ///< -1 left, 1 right, 0 none.
};

/// Remembers which cardinal direction is held on the gamepad and how long until the next repeat.
struct MenuRepeat {
    int heldMove = 0;
    int heldAdjust = 0;
    float secondsUntilNext = 0.0f;
};

/// Records any direction already held when a menu opens without firing an immediate step, so pausing
/// mid-run with the left stick pushed does not jump off the first item on the opening frame.
void LatchMenuRepeat(MenuRepeat& repeat, Vec2 direction, const MenuRepeatStats& stats = {});

/// Turns a continuous 2D input (left stick plus d-pad) into discrete menu steps with hysteresis and
/// key-like hold repeat. Picks the dominant axis so a diagonal push in Settings never changes a value
/// while moving to another row.
[[nodiscard]] MenuRepeatStep StepMenuRepeat(MenuRepeat& repeat, Vec2 direction, float dt,
                                            const MenuRepeatStats& stats = {});

} // namespace horde
