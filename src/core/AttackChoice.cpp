#include "core/AttackChoice.h"

#include <algorithm>

namespace horde {

namespace {

constexpr std::array<AttackKind, 3> kAttackKinds{AttackKind::Charge, AttackKind::HairRing, AttackKind::Leap};

const AttackWeights& WeightsOf(const AttackChoiceStats& stats, AttackKind kind) {
    switch (kind) {
    case AttackKind::Charge:
        return stats.charge;
    case AttackKind::HairRing:
        return stats.hairRing;
    case AttackKind::Leap:
        return stats.leap;
    }
    return stats.charge;
}

float Blend(float from, float to, float t) {
    return from + (to - from) * std::clamp(t, 0.0f, 1.0f);
}

} // namespace

float AttackWeightAt(const AttackChoiceStats& stats, AttackKind kind, float distance) {
    const AttackWeights& weights = WeightsOf(stats, kind);
    const float midDistance = 0.5f * (stats.nearDistance + stats.farDistance);
    if (distance <= midDistance) {
        const float span = std::max(midDistance - stats.nearDistance, 1.0f);
        return Blend(weights.nearWeight, weights.midWeight, (distance - stats.nearDistance) / span);
    }
    const float span = std::max(stats.farDistance - midDistance, 1.0f);
    return Blend(weights.midWeight, weights.farWeight, (distance - midDistance) / span);
}

AttackKind ChooseAttack(const AttackChoiceStats& stats, float distance, std::optional<AttackKind> last,
                        Random& random) {
    std::array<float, kAttackKinds.size()> weights{};
    float total = 0.0f;
    for (std::size_t i = 0; i < kAttackKinds.size(); ++i) {
        const AttackKind kind = kAttackKinds[i];
        weights[i] = kind == last ? 0.0f : std::max(AttackWeightAt(stats, kind, distance), 0.0f);
        total += weights[i];
    }
    // One draw, walked along the weights: each attack owns a stretch of [0, total) as long as its weight.
    // The draw is taken even when the answer is certain, so the numbers that follow do not depend on it.
    float draw = RandomRange(random, 0.0f, total);
    for (std::size_t i = 0; i < kAttackKinds.size(); ++i) {
        if (weights[i] > 0.0f && draw < weights[i]) {
            return kAttackKinds[i];
        }
        draw -= weights[i];
    }
    // Only reached through rounding at the very end of the range, where the draw belongs to the last attack
    // with any weight, or with every weight at 0, where any attack but the last one will do.
    for (std::size_t i = kAttackKinds.size(); i-- > 0;) {
        if (weights[i] > 0.0f) {
            return kAttackKinds[i];
        }
    }
    return kAttackKinds[0] == last ? kAttackKinds[1] : kAttackKinds[0];
}

std::optional<AttackKind> ChooseFollowUp(const ComboStats& stats, AttackKind opener, Random& random) {
    std::array<AttackKind, kCombos.size()> followUps{};
    std::size_t count = 0;
    for (const Combo& combo : kCombos) {
        if (combo.opener == opener) {
            followUps[count++] = combo.followUp;
        }
    }
    if (count == 0 || !Chance(random, stats.chance)) {
        return std::nullopt;
    }
    const auto pick = static_cast<std::size_t>(NextU32(random) % count);
    return followUps[pick];
}

} // namespace horde
