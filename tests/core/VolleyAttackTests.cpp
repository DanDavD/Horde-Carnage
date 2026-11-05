#include <vector>

#include <doctest/doctest.h>

#include "TestAttacker.h"
#include "core/DamageSource.h"
#include "core/attacks/Volley.h"

using horde::BossAttack;
using horde::VolleyStats;
using horde::Vec2;

namespace {

constexpr float kDt = horde::test::kAttackDt;
constexpr horde::Arena kArena{{0.0f, 0.0f}, {2000.0f, 1000.0f}};

// Numbers that are not the Matriarch's: a quarter of a second of warning, three bursts a quarter of a second
// apart, an eighth after the last and a quarter to recover, with slower strands.
constexpr VolleyStats kSlowVolley{.warningSeconds = 0.25f,
                                  .bursts = 3,
                                  .burstIntervalSeconds = 0.25f,
                                  .maxLeadSeconds = 1.0f,
                                  .activeSeconds = 0.125f,
                                  .recoverySeconds = 0.25f,
                                  .speed = 400.0f};

using Body = horde::test::TestBody;
using horde::test::BodyAt;

} // namespace

TEST_CASE("a volley with its own numbers fires its bursts on their beats, the middle one led") {
    Body body = BodyAt(Vec2{500.0f, 500.0f});
    BossAttack volley;
    // A player running south: the even bursts aim where they are, the odd one where they are going.
    horde::Motion player = horde::MotionAt(Vec2{800.0f, 500.0f});
    player.velocity = Vec2{0.0f, 100.0f};

    std::vector<int> spitSteps;
    std::vector<Vec2> aims;
    int finishedStep = 0;
    for (int stepIndex = 1; stepIndex <= 100 && finishedStep == 0; ++stepIndex) {
        const horde::VolleyStep step = horde::StepVolley(volley, body.AsAttacker(), kSlowVolley, player, kArena, kDt);
        if (step.spit) {
            spitSteps.push_back(stepIndex);
            aims.push_back(volley.direction);
        }
        if (step.finished) {
            finishedStep = stepIndex;
        }
    }

    CHECK(spitSteps == std::vector<int>{17, 32, 48});
    REQUIRE(aims.size() == 3);
    CHECK(aims[0] == Vec2{1.0f, 0.0f});
    CHECK(aims[1].y > 0.0f);
    const Vec2 lead = horde::LeadPoint(kSlowVolley, body.motion.position, body.radius, player, kArena);
    CHECK(aims[1] == horde::AimAt(body.motion.position, lead, Vec2{}));
    CHECK(aims[2] == Vec2{1.0f, 0.0f});
    CHECK(finishedStep == 16 + 40 + 16);
}

TEST_CASE("a volley leads a moving player by at most its cap, and never past the walls") {
    horde::Motion player = horde::MotionAt(Vec2{800.0f, 500.0f});
    CHECK(horde::LeadPoint(kSlowVolley, Vec2{500.0f, 500.0f}, 30.0f, player, kArena) == player.position);

    player.velocity = Vec2{0.0f, 100.0f};
    const VolleyStats capped{.maxLeadSeconds = 0.25f, .speed = 400.0f};
    CHECK(horde::LeadPoint(capped, Vec2{500.0f, 500.0f}, 30.0f, player, kArena) == Vec2{800.0f, 525.0f});

    player.position = Vec2{800.0f, 990.0f};
    CHECK(horde::LeadPoint(kSlowVolley, Vec2{500.0f, 500.0f}, 30.0f, player, kArena).y == kArena.max.y);
}

TEST_CASE("a volley's strands leave the edge of whatever body fires them, and count as the volley's") {
    const horde::Projectile strand =
        horde::MakeVolleyStrand(kSlowVolley, Vec2{100.0f, 100.0f}, 40.0f, Vec2{1.0f, 0.0f});
    CHECK(strand.motion.position == Vec2{140.0f, 100.0f});
    CHECK(strand.motion.velocity == Vec2{kSlowVolley.speed, 0.0f});
    CHECK(strand.source == horde::DamageSource::Volley);
}
