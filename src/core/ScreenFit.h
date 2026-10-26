#pragma once

#include "core/Vec2.h"

namespace horde {

/// How the game's own resolution sits on the real screen.
///
/// The game is laid out for one resolution (1280x720): the camera, the HUD and every distance the player
/// sees. On a screen of another size it is drawn at the screen's own resolution, scaled up or down as a
/// whole and centred, with black bars where the shapes differ. Everyone sees the same part of the arena,
/// so the fight is the same on every screen.
struct ScreenFit {
    float scale = 1.0f; ///< Screen pixels per game pixel.
    Vec2 offset;        ///< Where the game's top left corner lands on the screen, in screen pixels.
};

/// The largest scale at which `gameSize` fits inside `screenSize` whole, and the offset that centres it.
/// A screen with no area gives the identity fit.
[[nodiscard]] ScreenFit FitScreen(Vec2 gameSize, Vec2 screenSize);

/// A point on the screen (the mouse) in game pixels. Points on the black bars land outside the game's
/// area; the caller decides whether to clamp them.
[[nodiscard]] Vec2 ScreenToGame(const ScreenFit& fit, Vec2 screenPoint);

/// A point in game pixels on the screen.
[[nodiscard]] Vec2 GameToScreen(const ScreenFit& fit, Vec2 gamePoint);

} // namespace horde
