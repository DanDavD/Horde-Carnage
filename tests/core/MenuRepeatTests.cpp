#include <doctest/doctest.h>

#include "core/MenuRepeat.h"

using horde::MenuRepeat;
using horde::MenuRepeatStep;

TEST_CASE("menu repeat fires at once on tilt, waits for initial delay, then repeats steadily") {
    MenuRepeat repeat;

    CHECK(horde::StepMenuRepeat(repeat, {}, 0.016f).move == 0);
    CHECK(horde::StepMenuRepeat(repeat, {}, 0.016f).adjust == 0);

    // First push down steps immediately.
    MenuRepeatStep step = horde::StepMenuRepeat(repeat, {0.0f, 0.7f}, 0.016f);
    CHECK(step.move == 1);
    CHECK(step.adjust == 0);

    // Nothing fires while the initial delay (0.38 s) is still counting down.
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.7f}, 0.20f).move == 0);
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.7f}, 0.17f).move == 0);

    // Crossing 0.38 s fires the first repeat, and every 0.11 s after fires another.
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.7f}, 0.02f).move == 1);
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.7f}, 0.05f).move == 0);
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.7f}, 0.06f).move == 1);
}

TEST_CASE("hysteresis ignores thumb tremor near the threshold while releasing re-arms at once") {
    MenuRepeat repeat;

    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.6f}, 0.016f).move == -1);

    // Dipping between engage (0.50) and release (0.35) stays held without double-ticking.
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.40f}, 0.05f).move == 0);
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.58f}, 0.05f).move == 0);

    // Letting go below 0.35 resets immediately, so the next tap fires without waiting on the timer.
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.20f}, 0.016f).move == 0);
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.60f}, 0.016f).move == -1);
}

TEST_CASE("dominant axis picks either move or adjust, never both on a diagonal") {
    MenuRepeat repeat;

    SUBCASE("mostly vertical diagonal moves rows without adjusting") {
        const MenuRepeatStep step = horde::StepMenuRepeat(repeat, {0.55f, 0.80f}, 0.016f);
        CHECK(step.move == 1);
        CHECK(step.adjust == 0);
    }

    SUBCASE("mostly horizontal diagonal adjusts values without moving rows") {
        const MenuRepeatStep step = horde::StepMenuRepeat(repeat, {-0.85f, 0.55f}, 0.016f);
        CHECK(step.move == 0);
        CHECK(step.adjust == -1);
    }

    SUBCASE("rolling from vertical onto horizontal fires the new axis at once") {
        CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.80f}, 0.016f).move == 1);
        const MenuRepeatStep rolled = horde::StepMenuRepeat(repeat, {0.85f, 0.20f}, 0.016f);
        CHECK(rolled.move == 0);
        CHECK(rolled.adjust == 1);
    }

    SUBCASE("snapping to the opposite direction fires at once") {
        CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.80f}, 0.016f).move == -1);
        CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.80f}, 0.016f).move == 1);
    }
}

TEST_CASE("latching a held stick on menu open suppresses the opening jump") {
    MenuRepeat repeat;
    horde::LatchMenuRepeat(repeat, {0.0f, -0.9f});

    // Frame 0 of the newly opened menu stays on its initial item.
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.9f}, 0.016f).move == 0);

    // Keeping it held intentionally past initialDelay eventually steps; releasing re-arms cleanly.
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.9f}, 0.20f).move == 0);
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, -0.9f}, 0.18f).move == -1);

    horde::LatchMenuRepeat(repeat, {});
    CHECK(horde::StepMenuRepeat(repeat, {0.0f, 0.7f}, 0.016f).move == 1);
}

TEST_CASE("huge frame delta clamps to at most one repeated step") {
    MenuRepeat repeat;
    CHECK(horde::StepMenuRepeat(repeat, {1.0f, 0.0f}, 0.016f).adjust == 1);
    CHECK(horde::StepMenuRepeat(repeat, {1.0f, 0.0f}, 0.20f).adjust == 0);

    // Even after a multi-second hitch, only one step fires and the next frame waits a full interval.
    CHECK(horde::StepMenuRepeat(repeat, {1.0f, 0.0f}, 2.0f).adjust == 1);
    CHECK(horde::StepMenuRepeat(repeat, {1.0f, 0.0f}, 0.02f).adjust == 0);
}
