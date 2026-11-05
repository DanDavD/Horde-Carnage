#pragma once

namespace horde {

/// Which boss a fight is against. Each one has a row in the table of `BossRules` (Boss.h) and its rules in a
/// file of its own in `core/bosses/`.
enum class BossId {
    Matriarch, ///< The giant hairball: see bosses/Matriarch.h.
};

/// How many bosses there are: the size of the table of `BossRules`.
inline constexpr int kBossCount = 1;

} // namespace horde
