#pragma once

#include <raylib.h>

#include "core/Arena.h"
#include "core/StoneFloor.h"
#include "core/Vec2.h"

namespace horde {

/// Everything the drawing needs that is made once, when the game starts: the fonts, the textures made in
/// code, and the layout of the floor.
///
/// raylib can only make textures while its window is open, so `Game` loads this right after the window
/// opens and unloads it right before the window closes. It is plain data: copying it copies the handles,
/// not the textures, and only `Game` ever owns one.
struct Art {
    Font titleFont{};     ///< Cinzel Regular, loaded large: YOU DIED, VICTORY and the boss's name.
    Font uiFont{};        ///< Cinzel Bold, which holds up better at small sizes: the rest of the HUD.
    Texture2D grain{};    ///< Grey stone grain that tiles, multiplied over the floor.
    Texture2D glow{};     ///< A soft white dot, tinted for shadows, stains and the torch's warmth.
    Texture2D light{};    ///< The torch's reach: clear in the middle, opaque at the edge.
    Texture2D vignette{}; ///< Clear in the middle of the screen, opaque in the corners.
    StoneFloor floor;     ///< The slabs and stains of the arena's floor.
};

/// Makes the fonts and textures, and lays the floor of `arena`. Needs an open window.
[[nodiscard]] Art LoadArt(const Arena& arena);

/// Frees what `LoadArt` made. Needs the window still open.
void UnloadArt(Art& art);

/// The glow texture as a soft round blot of `radius` at `center`, in `color`.
void DrawGlow(const Art& art, Vec2 center, float radius, Color color);

} // namespace horde
