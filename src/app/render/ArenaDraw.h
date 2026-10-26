#pragma once

#include "app/render/Art.h"
#include "core/Arena.h"
#include "core/Vec2.h"

namespace horde {

/// The arena's floor, in world space: the stone slabs with their bevels and cracks, the grain and the stains.
void DrawFloor(const Arena& arena, const Art& art);

/// The walls, in world space: a band of dark stone blocks around the arena, with a pale lip where the light
/// catches their top edge, and the floor darkening at their foot.
void DrawWalls(const Arena& arena);

/// A soft shadow on the floor under a body of `radius` at `position`, thrown away from the light at `light`.
void DrawBodyShadow(Vec2 position, float radius, Vec2 light, const Art& art);

/// The player's torch, in world space: everything in `viewMin` to `viewMax` darkens with the distance from
/// `torch`, and warm light pools around it. It flickers with `seconds`.
void DrawTorchLight(Vec2 torch, Vec2 viewMin, Vec2 viewMax, float seconds, const Art& art);

/// The screen's corners darkened, in game pixels, over the world and under the HUD.
void DrawVignette(const Art& art);

} // namespace horde
