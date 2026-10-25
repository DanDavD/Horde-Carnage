#pragma once

#include <array>
#include <cstddef>

#include "core/Arena.h"
#include "core/Vec2.h"

namespace horde {

/// A mark on the floor where a summoned hairball is about to come out. It warns the player for a moment,
/// then the hairball appears on it and the mark is gone.
struct SpawnMarker {
    Vec2 position;
    float secondsLeft = 0.0f; ///< Until the hairball comes out.
    float totalSeconds = 0.0f; ///< The whole wait, for drawing how close it is.
};

/// The most points one summon can ask for.
inline constexpr std::size_t kMaxSummonPoints = 8;

/// The numbers of where a summon puts its hairballs.
struct SummonPlacement {
    int count = 4; ///< Points, evenly spaced around the player.
    /// How far from the player. Close enough that running away from the boss runs into them, far enough
    /// that a hairball never comes out next to the player.
    float distance = 250.0f;
    /// Kept this far inside the walls, so a hairball comes out whole.
    float wallMargin = 40.0f;
    /// A point pushed inside the walls that ends up closer to the player than this is dropped: near a corner
    /// a summon brings fewer hairballs rather than one on top of the player.
    float minPlayerDistance = 150.0f;
    /// A point closer than this to the boss's centre is dropped too, so no hairball comes out inside the boss
    /// (its radius, a hairball's and a little room).
    float minBossDistance = 100.0f;
};

/// Where one summon's hairballs come out.
struct SummonPoints {
    std::array<Vec2, kMaxSummonPoints> points{};
    std::size_t count = 0;
};

/// The points of a summon around `player`: `placement.count` of them evenly spaced on a circle of
/// `placement.distance`, turned by `turn` radians, each pushed inside the walls. A player who keeps to a wall
/// or a corner gets them along that wall, between the player and the open arena. Points that end up on top
/// of the player or inside the boss, standing at `boss`, are dropped.
[[nodiscard]] SummonPoints PlanSummon(Vec2 player, Vec2 boss, const Arena& arena, const SummonPlacement& placement,
                                      float turn);

/// A mark at `position` that lets its hairball out after `seconds`.
[[nodiscard]] SpawnMarker MakeSpawnMarker(Vec2 position, float seconds);

/// Counts a mark down by `dt` seconds.
///
/// @return Whether it is still waiting. False in the step its hairball comes out.
bool StepSpawnMarker(SpawnMarker& marker, float dt);

} // namespace horde
