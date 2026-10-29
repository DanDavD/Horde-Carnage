#pragma once

#include "app/render/Art.h"
#include "app/render/InputPrompts.h"
#include "core/TrailingBar.h"
#include "core/Vec2.h"
#include "core/World.h"

namespace horde {

/// The boss's name, on its bar and on the end screens.
inline constexpr const char* kBossName = "The Matriarch";

/// The trails behind the two health bars, which the game keeps from step to step.
struct HealthTrails {
    TrailingBar player;
    TrailingBar boss;
};

/// Everything on the screen that is not the world, in game pixels: the player's bars and flasks, the weapon
/// panel, the boss's bar and shield with the damage of the last run of hits, the frame rate, the version and
/// the attempt. While `playing`, also the controls line and the crosshair at `aim`.
void DrawHud(const World& world, const HealthTrails& trails, int attempt, bool playing, Vec2 aim,
             InputPromptStyle prompts, const Art& art);

} // namespace horde
