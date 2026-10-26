#pragma once

#include "app/render/Art.h"
#include "core/Vec2.h"
#include "core/World.h"

namespace horde {

/// The boss's name, on its bar and on the end screens.
inline constexpr const char* kBossName = "The Matriarch";

/// Everything on the screen that is not the world, in game pixels: the player's bars and flasks, the weapon
/// panel, the boss's bar and shield, the frame rate, the version and the attempt. While `playing`, also the
/// controls line and the crosshair at `aim`.
void DrawHud(const World& world, int attempt, bool playing, Vec2 aim, const Art& art);

} // namespace horde
