#include <doctest/doctest.h>

#include "core/FixedTimestep.h"

using horde::FixedTimestep;

// A step of 0.25 s is exact in binary floating point, so these tests compare without rounding noise.

TEST_CASE("one frame of exactly one step runs one step") {
    FixedTimestep timestep(0.25, 1.0);
    CHECK(timestep.Advance(0.25) == 1);
    CHECK(timestep.Alpha() == doctest::Approx(0.0));
}

TEST_CASE("short frames accumulate until a full step is ready") {
    FixedTimestep timestep(0.25, 1.0);
    CHECK(timestep.Advance(0.125) == 0);
    CHECK(timestep.Alpha() == doctest::Approx(0.5));
    CHECK(timestep.Advance(0.125) == 1);
}

TEST_CASE("a long frame runs several steps and keeps the remainder") {
    FixedTimestep timestep(0.25, 1.0);
    CHECK(timestep.Advance(0.625) == 2);
    CHECK(timestep.Alpha() == doctest::Approx(0.5));
}

TEST_CASE("a huge frame is clamped instead of running hundreds of catch-up steps") {
    FixedTimestep timestep(0.25, 1.0);
    CHECK(timestep.Advance(30.0) == 4);
}

TEST_CASE("a negative frame time never runs the simulation") {
    FixedTimestep timestep(0.25, 1.0);
    CHECK(timestep.Advance(-1.0) == 0);
    CHECK(timestep.Alpha() == doctest::Approx(0.0));
}

TEST_CASE("the default step is 60 Hz") {
    FixedTimestep timestep;
    CHECK(timestep.StepSeconds() == doctest::Approx(1.0 / 60.0));
    CHECK(timestep.Advance(1.0 / 60.0) == 1);
}
