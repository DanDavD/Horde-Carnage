#include "core/StoneFloor.h"

#include <algorithm>
#include <cmath>

#include "core/Random.h"

namespace horde {

namespace {

// Its own stream, so the floor never shares numbers with anything else seeded the same way.
constexpr std::uint64_t kFloorStream = 11;

Vec2 PointIn(Random& random, Vec2 min, Vec2 size, float xFrom, float xTo) {
    return min + Vec2{size.x * RandomRange(random, xFrom, xTo), size.y * RandomRange(random, 0.15f, 0.85f)};
}

void Crack(Slab& slab, Random& random) {
    const Vec2 size = slab.max - slab.min;
    slab.cracked = true;
    slab.crack = {PointIn(random, slab.min, size, 0.1f, 0.3f), PointIn(random, slab.min, size, 0.4f, 0.6f),
                  PointIn(random, slab.min, size, 0.7f, 0.9f)};
}

} // namespace

StoneFloor LayStoneFloor(const Arena& arena, std::uint64_t seed, const StoneFloorStats& stats) {
    Random random = SeedRandom(seed, kFloorStream);
    StoneFloor floor;
    const Vec2 size = arena.max - arena.min;
    const int rows = std::max(1, static_cast<int>(std::lround(size.y / stats.rowHeight)));

    const float rowHeight = size.y / static_cast<float>(rows);
    for (int row = 0; row < rows; ++row) {
        const float top = arena.min.y + rowHeight * static_cast<float>(row);
        // The last row ends exactly on the wall, whatever the rounding.
        const float bottom = row + 1 == rows ? arena.max.y : top + rowHeight;
        float left = arena.min.x;
        while (left < arena.max.x) {
            const float remaining = arena.max.x - left;
            // A slab never leaves less than `minWidth` behind it, so the row cannot end on a sliver.
            const bool last = remaining <= stats.maxWidth;
            const float widest = std::max(stats.minWidth, std::min(stats.maxWidth, remaining - stats.minWidth));
            const float right = last ? arena.max.x : left + RandomRange(random, stats.minWidth, widest);
            Slab slab{.min = {left, top}, .max = {right, bottom}, .tone = RandomRange(random, -1.0f, 1.0f)};
            if (Chance(random, stats.crackChance)) {
                Crack(slab, random);
            }
            floor.slabs.push_back(slab);
            left = right;
        }
    }

    for (int i = 0; i < stats.stains; ++i) {
        const Vec2 center{RandomRange(random, arena.min.x, arena.max.x), RandomRange(random, arena.min.y, arena.max.y)};
        floor.stains.push_back(Stain{center, RandomRange(random, stats.minStainRadius, stats.maxStainRadius)});
    }
    return floor;
}

} // namespace horde
