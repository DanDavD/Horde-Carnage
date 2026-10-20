#include "core/FixedTimestep.h"

#include <algorithm>
#include <cassert>

namespace horde {

FixedTimestep::FixedTimestep(double stepSeconds, double maxFrameSeconds)
    : _stepSeconds(stepSeconds), _maxFrameSeconds(std::max(maxFrameSeconds, stepSeconds)) {
    assert(stepSeconds > 0.0 && "FixedTimestep needs a positive step");
}

int FixedTimestep::Advance(double frameSeconds) {
    // A negative time can come from a bad clock; it must never run the simulation backwards.
    _accumulator += std::clamp(frameSeconds, 0.0, _maxFrameSeconds);

    int steps = 0;
    while (_accumulator >= _stepSeconds) {
        _accumulator -= _stepSeconds;
        ++steps;
    }
    return steps;
}

double FixedTimestep::Alpha() const {
    return _accumulator / _stepSeconds;
}

} // namespace horde
