#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "core/Arena.h"
#include "core/Vec2.h"

namespace horde {

/// How the stone floor is laid, in world units.
struct StoneFloorStats {
    float rowHeight = 80.0f; ///< Rows are as close to this as divides the arena evenly.
    float minWidth = 90.0f;  ///< No slab is narrower, unless the whole arena is.
    float maxWidth = 220.0f; ///< At least twice `minWidth`, so a row can always end on a whole slab.
    float crackChance = 0.18f;
    int stains = 9; ///< Dark stains of old blood.
    float minStainRadius = 30.0f;
    float maxStainRadius = 90.0f;
};

/// One stone slab: a rectangle of the floor, a little lighter or darker than the rest.
struct Slab {
    Vec2 min;
    Vec2 max;
    float tone = 0.0f; ///< From -1, the darkest stone, to 1, the lightest.
    bool cracked = false;
    std::array<Vec2, 3> crack{}; ///< A crack's three points, left to right inside the slab, when `cracked`.
};

/// A dark stain on the floor.
struct Stain {
    Vec2 center;
    float radius = 0.0f;
};

/// The floor of an arena, laid once when the game starts and drawn every frame.
struct StoneFloor {
    std::vector<Slab> slabs;
    std::vector<Stain> stains;
};

/// Lays the floor of `arena`: rows of slabs of random widths that cover it exactly once, so the joints of
/// one row rarely meet those of the next, as in a floor laid by hand. The same seed always lays the same
/// floor, so the arena looks the same in every fight.
[[nodiscard]] StoneFloor LayStoneFloor(const Arena& arena, std::uint64_t seed, const StoneFloorStats& stats = {});

} // namespace horde
