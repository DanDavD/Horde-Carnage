#pragma once

#include <raylib.h>

#include "core/FollowCamera.h"
#include "core/ScreenFit.h"
#include "core/Vec2.h"

namespace horde {

// The game's own resolution. Everything is laid out for it, and the whole picture is scaled to the real
// screen (see ScreenFit), so every screen shows the same part of the arena.
inline constexpr int kScreenWidth = 1280;
inline constexpr int kScreenHeight = 720;

// At zoom 1 one world unit is one pixel, so the view is the screen size.
inline constexpr Vec2 kViewSize{static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight)};

/// Where the game sits on the screen this frame. The screen's size can change (a browser window resized,
/// itch.io going fullscreen), so it is worked out every frame.
[[nodiscard]] ScreenFit CurrentScreenFit();

/// The mouse in game pixels, kept inside the game: a mouse over the black bars aims at the edge.
[[nodiscard]] Vec2 AimInGame(const ScreenFit& fit);

/// The screen-space camera for everything drawn in game pixels that does not move with the world: the HUD,
/// the end screens and the crosshair.
[[nodiscard]] Camera2D ScreenCamera(const ScreenFit& fit);

/// The follow camera at the game's own resolution, interpolated by `alpha`. The mouse is turned into a world
/// point through it.
[[nodiscard]] Camera2D ToRaylibCamera(const FollowCamera& camera, float alpha);

/// The follow camera moved and scaled onto the real screen, for drawing the world.
[[nodiscard]] Camera2D WorldCamera(const FollowCamera& camera, float alpha, const ScreenFit& fit);

} // namespace horde
