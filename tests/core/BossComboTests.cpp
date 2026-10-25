#include <cstdint>
#include <set>
#include <vector>

#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/Boss.h"
#include "core/World.h"

using horde::AttackKind;
using horde::AttackStage;
using horde::Boss;
using horde::BossAttack;
using horde::Random;
using horde::Vec2;

namespace {

// The game's arena: (0, 0) to (1640, 1040), centre (820, 520).
constexpr horde::Arena kArena = horde::kBossArena;
constexpr Vec2 kCenter{820.0f, 520.0f};
constexpr float kDt = 1.0f / 60.0f;
constexpr float kPlayerRadius = 20.0f;
constexpr Vec2 kEast = kCenter + Vec2{300.0f, 0.0f};
constexpr Vec2 kSouth = kCenter + Vec2{0.0f, 300.0f};

// A boss without its chance standing in the middle, at the very start of `attack`.
Boss StartingWith(BossAttack attack) {
    Boss boss = horde::test::Predictable(Boss{});
    boss.motion = horde::MotionAt(kCenter);
    boss.nextAttack.reset();
    boss.lastAttack = attack.kind;
    boss.attack = attack;
    return boss;
}

// Steps the boss, and returns what it asked of the world in the last step.
horde::BossActions Step(Boss& boss, Vec2 player, Random& random, int steps) {
    horde::BossActions actions;
    for (int i = 0; i < steps; ++i) {
        actions = horde::StepBoss(boss, player, kPlayerRadius, kArena, random, kDt);
    }
    return actions;
}

// One attack as it started in a fight.
struct Started {
    BossAttack attack;
    float stalkBefore = 0.0f; ///< How long the boss stalked before it; 0 for the second attack of a combo.
};

// A whole fight of `seconds` against a player who stands south of the boss's start and never fights back,
// recording every attack as it starts.
std::vector<Started> PlayFight(Boss boss, std::uint64_t seed, float seconds) {
    Random random = horde::SeedRandom(seed);
    boss.motion = horde::MotionAt(kCenter);
    std::vector<Started> started;
    float stalk = 0.0f;
    bool attacking = false;
    for (int i = 0; i < static_cast<int>(seconds / kDt); ++i) {
        Step(boss, kSouth, random, 1);
        if (!boss.attack) {
            // In the step an attack ends, the boss sets itself the whole stalk that follows.
            if (attacking) {
                stalk = boss.secondsUntilAttack;
            }
            attacking = false;
            continue;
        }
        attacking = true;
        // An attack has not been stepped yet in the step it starts.
        if (boss.attack->stage == AttackStage::Warning && boss.attack->stageSeconds == 0.0f) {
            started.push_back({*boss.attack, boss.attack->endsCombo ? 0.0f : stalk});
        }
    }
    return started;
}

} // namespace

TEST_CASE("a fight opens with a charge, then never repeats an attack outside a combo") {
    for (std::uint64_t seed = 1; seed <= 5; ++seed) {
        Boss boss;
        boss.combo.chance = 0.0f;
        const std::vector<Started> started = PlayFight(boss, seed, 90.0f);
        REQUIRE(started.size() > 20);
        CHECK(started[0].attack.kind == AttackKind::Charge);
        std::set<AttackKind> kinds;
        for (std::size_t i = 1; i < started.size(); ++i) {
            CHECK(started[i].attack.kind != started[i - 1].attack.kind);
            kinds.insert(started[i].attack.kind);
        }
        // Every attack shows up.
        CHECK(kinds.size() == 3);
    }
}

TEST_CASE("the stalks and the warnings vary within their ranges") {
    const Boss boss;
    const std::vector<Started> started = PlayFight(boss, 21, 90.0f);
    REQUIRE(started.size() > 20);
    std::set<float> stalks;
    std::set<float> scales;
    for (std::size_t i = 1; i < started.size(); ++i) {
        const Started& one = started[i];
        if (one.attack.endsCombo) {
            continue;
        }
        CHECK(one.stalkBefore >= boss.choice.minStalkSeconds);
        CHECK(one.stalkBefore <= boss.choice.maxStalkSeconds);
        CHECK(one.attack.warningScale >= boss.choice.minWarningScale);
        CHECK(one.attack.warningScale <= boss.choice.maxWarningScale);
        stalks.insert(one.stalkBefore);
        scales.insert(one.attack.warningScale);
    }
    CHECK(stalks.size() > 10);
    CHECK(scales.size() > 10);
}

TEST_CASE("combos happen in a fight, and only the known ones") {
    const Boss boss;
    const std::vector<Started> started = PlayFight(boss, 31, 120.0f);
    int combos = 0;
    for (std::size_t i = 1; i < started.size(); ++i) {
        if (!started[i].attack.endsCombo) {
            continue;
        }
        ++combos;
        const AttackKind opener = started[i - 1].attack.kind;
        const AttackKind followUp = started[i].attack.kind;
        bool known = false;
        for (const horde::Combo& combo : horde::kCombos) {
            known = known || (combo.opener == opener && combo.followUp == followUp);
        }
        CHECK(known);
        CHECK(started[i - 1].attack.followUp == followUp);
    }
    CHECK(combos > 3);
}

TEST_CASE("the same seed plays the same fight") {
    const std::vector<Started> a = PlayFight(Boss{}, 41, 60.0f);
    const std::vector<Started> b = PlayFight(Boss{}, 41, 60.0f);
    const std::vector<Started> other = PlayFight(Boss{}, 42, 60.0f);
    REQUIRE(a.size() == b.size());
    bool differs = a.size() != other.size();
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].attack.kind == b[i].attack.kind);
        CHECK(a[i].attack.warningScale == b[i].attack.warningScale);
        CHECK(a[i].stalkBefore == b[i].stalkBefore);
        if (i < other.size()) {
            differs = differs || a[i].attack.kind != other[i].attack.kind ||
                      a[i].attack.warningScale != other[i].attack.warningScale;
        }
    }
    CHECK(differs);
}

TEST_CASE("in a combo the second attack starts in place of the first one's recovery, with a shorter warning") {
    // The Rebound: a charge east into the wall, and straight away a charge back at the player.
    Random random = horde::SeedRandom(51);
    Boss boss = StartingWith(BossAttack{.kind = AttackKind::Charge, .followUp = AttackKind::Charge});
    for (int i = 0; i < 120 && boss.attack && !boss.attack->endsCombo; ++i) {
        Step(boss, kEast, random, 1);
        REQUIRE(boss.attack);
        CHECK(boss.attack->stage != AttackStage::Recovery);
    }
    REQUIRE(boss.attack);
    const BossAttack& rebound = *boss.attack;
    CHECK(rebound.endsCombo);
    CHECK_FALSE(rebound.followUp);
    CHECK(rebound.kind == AttackKind::Charge);
    CHECK(rebound.stage == AttackStage::Warning);
    // Against the east wall, with no stun.
    CHECK(boss.motion.position.x == doctest::Approx(kArena.max.x - boss.radius));
    CHECK_FALSE(rebound.stunned);
    // 0.8 s at 0.7.
    CHECK(horde::WarningSeconds(boss, rebound) == doctest::Approx(0.56f));
    int warning = 0;
    while (boss.attack && boss.attack->stage == AttackStage::Warning && warning < 120) {
        Step(boss, kCenter, random, 1);
        ++warning;
    }
    CHECK(warning == doctest::Approx(34).epsilon(0.05));
    CHECK(boss.attack->direction.x == doctest::Approx(-1.0f));
}

TEST_CASE("the last attack of a combo rests for the combo's longer recovery") {
    // The Crash: a leap and, on landing, a hair ring. The ring rests 1.2 s instead of its own 0.8 s.
    Random random = horde::SeedRandom(52);
    Boss boss = StartingWith(BossAttack{.kind = AttackKind::Leap, .followUp = AttackKind::HairRing});
    boss.attack->takeoff = kCenter;
    boss.attack->target = kSouth;
    int landings = 0;
    int spits = 0;
    int stepsToSpit = 0;
    for (int i = 0; i < 300 && boss.attack && boss.attack->stage != AttackStage::Recovery; ++i) {
        const horde::BossActions actions = Step(boss, kSouth, random, 1);
        landings += actions.landed ? 1 : 0;
        spits += actions.spitHairRing ? 1 : 0;
        if (landings > 0 && spits == 0) {
            ++stepsToSpit;
        }
    }
    CHECK(landings == 1);
    CHECK(spits == 1);
    // 0.15 s of landing, then 0.6 s of ring warning at 0.7.
    CHECK(stepsToSpit == doctest::Approx(9 + 26).epsilon(0.05));
    REQUIRE(boss.attack);
    CHECK(boss.attack->kind == AttackKind::HairRing);
    CHECK(boss.attack->endsCombo);

    int recovery = 0;
    while (boss.attack && recovery < 200) {
        Step(boss, kSouth, random, 1);
        ++recovery;
    }
    CHECK(recovery == doctest::Approx(72).epsilon(0.05));
}

TEST_CASE("a longer or shorter warning keeps the charge's aim lock at its exact length") {
    Random random = horde::SeedRandom(53);
    // 0.8 s at 1.25: 1 s of warning, the last 0.3 s locked.
    Boss boss = StartingWith(BossAttack{.kind = AttackKind::Charge, .warningScale = 1.25f});
    const Vec2 north = kCenter + Vec2{0.0f, -300.0f};
    Step(boss, north, random, 40); // 0.67 s: still following.
    CHECK(boss.attack->direction.y == doctest::Approx(-1.0f));
    Step(boss, north, random, 3);
    Step(boss, kEast, random, 10); // From 0.72 s on, locked.
    REQUIRE(boss.attack->stage == AttackStage::Warning);
    CHECK(boss.attack->direction.y == doctest::Approx(-1.0f));
    CHECK(horde::WarningSeconds(boss, *boss.attack) == doctest::Approx(1.0f));
}

TEST_CASE("a shorter leap keeps the shadow's lock, and the boss is in the air after its shorter crouch") {
    Boss boss = horde::test::Predictable(Boss{});
    const BossAttack leap{.kind = AttackKind::Leap, .warningScale = 0.5f};
    CHECK(horde::LeapCrouchSeconds(boss, leap) == doctest::Approx(0.175f));
    CHECK(horde::LeapAirSeconds(boss, leap) == doctest::Approx(0.5f));
    // Never shorter than the lock, however short the warning.
    const BossAttack tiny{.kind = AttackKind::Leap, .warningScale = 0.1f};
    CHECK(horde::LeapAirSeconds(boss, tiny) == doctest::Approx(boss.leap.lockSeconds));
    const BossAttack tinyCharge{.kind = AttackKind::Charge, .warningScale = 0.1f};
    CHECK(horde::WarningSeconds(boss, tinyCharge) == doctest::Approx(boss.charge.aimLockSeconds));

    boss.attack = BossAttack{.kind = AttackKind::Leap, .stageSeconds = 0.2f, .warningScale = 0.5f};
    CHECK(horde::IsAirborne(boss));
    boss.attack->warningScale = 1.0f;
    CHECK_FALSE(horde::IsAirborne(boss));
}
