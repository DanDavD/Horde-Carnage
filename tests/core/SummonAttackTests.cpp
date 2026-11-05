#include <doctest/doctest.h>

#include "TestAttacker.h"
#include "core/attacks/Summon.h"

using horde::BossAttack;
using horde::SummonStats;
using horde::Vec2;

namespace {

constexpr float kDt = horde::test::kAttackDt;

// Numbers that are not the Matriarch's: a quarter of a second of howl (16 steps), an eighth active (8 steps)
// and a quarter to recover (16 steps).
constexpr SummonStats kShortHowl{.warningSeconds = 0.25f, .activeSeconds = 0.125f, .recoverySeconds = 0.25f};

using Body = horde::test::TestBody;
using horde::test::BodyAt;

} // namespace

TEST_CASE("a summon with its own numbers puts its marks down once, as the howl starts, then recovers") {
    Body body = BodyAt(Vec2{500.0f, 500.0f});
    BossAttack summon;

    int marks = 0;
    int marksStep = 0;
    int finishedStep = 0;
    for (int stepIndex = 1; stepIndex <= 100 && finishedStep == 0; ++stepIndex) {
        const horde::SummonStep step = horde::StepSummon(summon, body.AsAttacker(), kShortHowl, kDt);
        if (step.placeMarks) {
            ++marks;
            marksStep = stepIndex;
        }
        if (step.finished) {
            finishedStep = stepIndex;
        }
    }

    CHECK(marks == 1);
    CHECK(marksStep == 1);
    CHECK(finishedStep == 16 + 8 + 16);
    CHECK(body.motion.position == Vec2{500.0f, 500.0f});
}

TEST_CASE("a summon's howl is scaled by the attack") {
    BossAttack summon;
    summon.warningScale = 0.5f;
    CHECK(horde::SummonWarningSeconds(kShortHowl, summon) == 0.125f);
}
