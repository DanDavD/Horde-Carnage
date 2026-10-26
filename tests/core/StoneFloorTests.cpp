#include <doctest/doctest.h>

#include "core/StoneFloor.h"
#include "core/World.h"

using horde::Arena;
using horde::Slab;
using horde::StoneFloor;
using horde::StoneFloorStats;
using horde::Vec2;

namespace {

constexpr std::uint64_t kSeed = 2026;

bool Inside(const Slab& slab, Vec2 point) {
    return point.x >= slab.min.x && point.x < slab.max.x && point.y >= slab.min.y && point.y < slab.max.y;
}

} // namespace

TEST_CASE("the slabs cover the whole floor exactly once") {
    const Arena& arena = horde::kBossArena;
    const StoneFloor floor = horde::LayStoneFloor(arena, kSeed);
    REQUIRE_FALSE(floor.slabs.empty());

    float area = 0.0f;
    for (const Slab& slab : floor.slabs) {
        area += (slab.max.x - slab.min.x) * (slab.max.y - slab.min.y);
    }
    const Vec2 size = arena.max - arena.min;
    CHECK(area == doctest::Approx(size.x * size.y));

    // Every point of a fine grid lies in one slab, never in none and never in two.
    int gaps = 0;
    int overlaps = 0;
    for (float y = arena.min.y + 2.5f; y < arena.max.y; y += 5.0f) {
        for (float x = arena.min.x + 2.5f; x < arena.max.x; x += 5.0f) {
            int count = 0;
            for (const Slab& slab : floor.slabs) {
                count += Inside(slab, {x, y}) ? 1 : 0;
            }
            gaps += count == 0 ? 1 : 0;
            overlaps += count > 1 ? 1 : 0;
        }
    }
    CHECK(gaps == 0);
    CHECK(overlaps == 0);
}

TEST_CASE("every slab is between the narrowest and the widest, in rows of the same height") {
    const StoneFloorStats stats;
    const StoneFloor floor = horde::LayStoneFloor(horde::kBossArena, kSeed, stats);
    for (const Slab& slab : floor.slabs) {
        const float width = slab.max.x - slab.min.x;
        CHECK(width >= stats.minWidth - 0.01f);
        CHECK(width <= stats.maxWidth + 0.01f);
        // 1040 is 13 rows of exactly 80.
        CHECK(slab.max.y - slab.min.y == doctest::Approx(stats.rowHeight));
        CHECK(slab.tone >= -1.0f);
        CHECK(slab.tone <= 1.0f);
    }
}

TEST_CASE("the same seed lays the same floor, and another seed another") {
    const StoneFloor a = horde::LayStoneFloor(horde::kBossArena, kSeed);
    const StoneFloor b = horde::LayStoneFloor(horde::kBossArena, kSeed);
    const StoneFloor c = horde::LayStoneFloor(horde::kBossArena, kSeed + 1);
    REQUIRE(a.slabs.size() == b.slabs.size());
    for (std::size_t i = 0; i < a.slabs.size(); ++i) {
        CHECK(a.slabs[i].max == b.slabs[i].max);
        CHECK(a.slabs[i].tone == b.slabs[i].tone);
    }
    bool differs = a.slabs.size() != c.slabs.size();
    for (std::size_t i = 0; !differs && i < a.slabs.size(); ++i) {
        differs = a.slabs[i].max != c.slabs[i].max;
    }
    CHECK(differs);
}

TEST_CASE("cracks stay inside their slab and stains inside the arena") {
    const Arena& arena = horde::kBossArena;
    const StoneFloorStats stats;
    const StoneFloor floor = horde::LayStoneFloor(arena, kSeed, stats);
    int cracked = 0;
    for (const Slab& slab : floor.slabs) {
        if (!slab.cracked) {
            continue;
        }
        ++cracked;
        for (const Vec2 point : slab.crack) {
            CHECK(Inside(slab, point));
        }
        // It runs across the slab, left to right.
        CHECK(slab.crack[0].x < slab.crack[1].x);
        CHECK(slab.crack[1].x < slab.crack[2].x);
    }
    // About one slab in five, out of roughly a hundred.
    CHECK(cracked > 5);
    CHECK(cracked < static_cast<int>(floor.slabs.size()) / 2);

    REQUIRE(floor.stains.size() == static_cast<std::size_t>(stats.stains));
    for (const horde::Stain& stain : floor.stains) {
        CHECK(horde::Contains(arena, stain.center));
        CHECK(stain.radius >= stats.minStainRadius);
        CHECK(stain.radius <= stats.maxStainRadius);
    }
}

TEST_CASE("an arena narrower than one slab gets one slab per row") {
    const Arena narrow{{0.0f, 0.0f}, {50.0f, 160.0f}};
    const StoneFloor floor = horde::LayStoneFloor(narrow, kSeed);
    REQUIRE(floor.slabs.size() == 2);
    CHECK(floor.slabs[0].min == Vec2{0.0f, 0.0f});
    CHECK(floor.slabs[1].max == Vec2{50.0f, 160.0f});
}
