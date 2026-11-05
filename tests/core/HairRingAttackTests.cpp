#include <cmath>
#include <numbers>
#include <vector>

#include <doctest/doctest.h>

#include "TestAttacker.h"
#include "core/attacks/HairRing.h"

using horde::BossAttack;
using horde::HairRingStats;
using horde::Vec2;

namespace {

constexpr float kDt = horde::test::kAttackDt;

// Numbers that are not the Matriarch's: a quarter of a second of warning (16 steps), three rings a quarter of
// a second apart with the gap turning a right angle between them, an eighth of a second after the last, and
// a quarter to recover.
constexpr HairRingStats kTripleRing{.warningSeconds = 0.25f,
                                    .activeSeconds = 0.125f,
                                    .recoverySeconds = 0.25f,
                                    .waves = 3,
                                    .waveIntervalSeconds = 0.25f,
                                    .gapTurnDegrees = 90.0f};

using Body = horde::test::TestBody;
using horde::test::BodyAt;

bool Near(Vec2 a, Vec2 b) {
    return std::fabs(a.x - b.x) < 1e-5f && std::fabs(a.y - b.y) < 1e-5f;
}

} // namespace

TEST_CASE("a hair ring with its own numbers spits each ring on its beat, turning the gap between them") {
    Body body = BodyAt(Vec2{500.0f, 500.0f});
    BossAttack ring;
    ring.direction = Vec2{1.0f, 0.0f};

    std::vector<int> spitSteps;
    std::vector<Vec2> gaps;
    int finishedStep = 0;
    for (int stepIndex = 1; stepIndex <= 100 && finishedStep == 0; ++stepIndex) {
        const horde::HairRingStep step = horde::StepHairRing(ring, body.AsAttacker(), kTripleRing, kDt);
        if (step.spit) {
            spitSteps.push_back(stepIndex);
            gaps.push_back(ring.direction);
        }
        if (step.finished) {
            finishedStep = stepIndex;
        }
    }

    // The warning ends on step 16; the first ring leaves on the next step, the others a quarter second apart.
    CHECK(spitSteps == std::vector<int>{17, 32, 48});
    REQUIRE(gaps.size() == 3);
    CHECK(Near(gaps[0], Vec2{1.0f, 0.0f}));
    CHECK(Near(gaps[1], horde::Rotated(gaps[0], std::numbers::pi_v<float> / 2.0f)));
    CHECK(Near(gaps[2], horde::Rotated(gaps[1], std::numbers::pi_v<float> / 2.0f)));
    // The last ring at 0.5 s into the spit, an eighth after it, then a quarter of a second of recovery.
    CHECK(finishedStep == 16 + 40 + 16);
    CHECK(body.motion.position == Vec2{500.0f, 500.0f});
}

TEST_CASE("a hair ring's gap opens to one side of the player, or to the other") {
    BossAttack ring;
    const HairRingStats stats;
    horde::AimHairRing(ring, Vec2{1.0f, 0.0f}, false, stats);
    CHECK(Near(ring.direction, Vec2{0.0f, 1.0f}));
    horde::AimHairRing(ring, Vec2{1.0f, 0.0f}, true, stats);
    CHECK(Near(ring.direction, Vec2{0.0f, -1.0f}));
}

TEST_CASE("a hair ring's strands leave the edge of whatever body spits them, with the ring's numbers") {
    const HairRingStats stats{.slots = 8, .gapSlots = 2, .speed = 150.0f, .strandRadius = 5.0f, .damage = 4};
    CHECK(horde::HairRingStrands(stats) == 6);

    const horde::Projectile strand = horde::MakeHairStrand(stats, Vec2{100.0f, 100.0f}, 40.0f, Vec2{0.0f, 1.0f});
    CHECK(strand.motion.position == Vec2{100.0f, 140.0f});
    CHECK(strand.motion.velocity == Vec2{0.0f, 150.0f});
    CHECK(strand.radius == 5.0f);
    CHECK(strand.damage == 4);
}
