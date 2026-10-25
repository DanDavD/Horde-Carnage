#include <array>
#include <optional>

#include <doctest/doctest.h>

#include "core/AttackChoice.h"

using horde::AttackChoiceStats;
using horde::AttackKind;
using horde::ComboStats;
using horde::Random;

namespace {

constexpr int kPicks = 1000;

// How many of `kPicks` picks went to each attack, in the order Charge, Hair Ring, Leap.
std::array<int, 3> CountPicks(float distance, std::optional<AttackKind> last, Random& random) {
    std::array<int, 3> counts{};
    for (int i = 0; i < kPicks; ++i) {
        ++counts[static_cast<std::size_t>(horde::ChooseAttack(AttackChoiceStats{}, distance, last, random))];
    }
    return counts;
}

} // namespace

TEST_CASE("each attack weighs most at its own distance, and the weights blend in between") {
    const AttackChoiceStats stats;
    const auto weight = [&stats](AttackKind kind, float distance) {
        return horde::AttackWeightAt(stats, kind, distance);
    };
    // Up close, the Hair Ring; in the middle, the Leap; far away, the Charge.
    CHECK(weight(AttackKind::HairRing, 100.0f) == doctest::Approx(stats.hairRing.nearWeight));
    CHECK(weight(AttackKind::Leap, 400.0f) == doctest::Approx(stats.leap.midWeight));
    CHECK(weight(AttackKind::Charge, 900.0f) == doctest::Approx(stats.charge.farWeight));
    CHECK(weight(AttackKind::HairRing, 100.0f) > weight(AttackKind::Charge, 100.0f));
    CHECK(weight(AttackKind::Leap, 400.0f) > weight(AttackKind::HairRing, 400.0f));
    CHECK(weight(AttackKind::Charge, 900.0f) > weight(AttackKind::Leap, 900.0f));

    // Halfway between the near distance (250) and the middle (400), halfway between the two weights.
    CHECK(weight(AttackKind::Charge, 325.0f) == doctest::Approx(1.5f));
    CHECK(weight(AttackKind::Leap, 475.0f) == doctest::Approx(3.0f));
}

TEST_CASE("the attack that just ended is never picked again") {
    Random random = horde::SeedRandom(11);
    for (const AttackKind last : {AttackKind::Charge, AttackKind::HairRing, AttackKind::Leap}) {
        for (const float distance : {100.0f, 400.0f, 900.0f}) {
            const std::array<int, 3> counts = CountPicks(distance, last, random);
            CHECK(counts[static_cast<std::size_t>(last)] == 0);
        }
    }
}

TEST_CASE("the distance shifts the picks toward the attack that suits it") {
    Random random = horde::SeedRandom(12);
    // Up close the weights are 1, 4 and 1: two picks in three are a Hair Ring.
    const std::array<int, 3> closeUp = CountPicks(100.0f, std::nullopt, random);
    CHECK(closeUp[1] > 600);
    CHECK(closeUp[1] < 733);
    // Far away they are 4, 1 and 2: four in seven are a Charge.
    const std::array<int, 3> farAway = CountPicks(900.0f, std::nullopt, random);
    CHECK(farAway[0] > 500);
    CHECK(farAway[0] < 643);
    CHECK(farAway[2] > farAway[1]);
    // With the Charge just ended, far away the Leap takes two in three.
    const std::array<int, 3> afterCharge = CountPicks(900.0f, AttackKind::Charge, random);
    CHECK(afterCharge[2] > 600);
    CHECK(afterCharge[2] < 733);
}

TEST_CASE("the same seed picks the same attacks") {
    Random a = horde::SeedRandom(13);
    Random b = horde::SeedRandom(13);
    std::optional<AttackKind> last;
    for (int i = 0; i < 50; ++i) {
        const float distance = 50.0f * static_cast<float>(i % 20);
        const AttackKind pick = horde::ChooseAttack(AttackChoiceStats{}, distance, last, a);
        CHECK(pick == horde::ChooseAttack(AttackChoiceStats{}, distance, last, b));
        last = pick;
    }
}

TEST_CASE("a charge or a leap opens a combo about a third of the time, and a hair ring never") {
    Random random = horde::SeedRandom(14);
    const ComboStats stats;
    int combos = 0;
    int rebounds = 0;
    int pursuits = 0;
    for (int i = 0; i < kPicks; ++i) {
        const std::optional<AttackKind> followUp = horde::ChooseFollowUp(stats, AttackKind::Charge, random);
        combos += followUp ? 1 : 0;
        rebounds += followUp == AttackKind::Charge ? 1 : 0;
        pursuits += followUp == AttackKind::Leap ? 1 : 0;
    }
    // 35% of 1000, with both of the Charge's combos picked about as often.
    CHECK(combos > 300);
    CHECK(combos < 400);
    CHECK(rebounds + pursuits == combos);
    CHECK(rebounds > 120);
    CHECK(pursuits > 120);

    // The Leap has one combo, the Crash into a Hair Ring.
    int crashes = 0;
    int others = 0;
    for (int i = 0; i < kPicks; ++i) {
        const std::optional<AttackKind> afterLeap = horde::ChooseFollowUp(stats, AttackKind::Leap, random);
        crashes += afterLeap == AttackKind::HairRing ? 1 : 0;
        others += afterLeap && afterLeap != AttackKind::HairRing ? 1 : 0;
        others += horde::ChooseFollowUp(stats, AttackKind::HairRing, random) ? 1 : 0;
    }
    CHECK(crashes > 300);
    CHECK(crashes < 400);
    CHECK(others == 0);
}

TEST_CASE("a combo chance of 0 never chains, and of 1 always does") {
    Random random = horde::SeedRandom(15);
    for (int i = 0; i < 100; ++i) {
        CHECK_FALSE(horde::ChooseFollowUp(ComboStats{.chance = 0.0f}, AttackKind::Charge, random));
        CHECK(horde::ChooseFollowUp(ComboStats{.chance = 1.0f}, AttackKind::Leap, random) == AttackKind::HairRing);
    }
}
