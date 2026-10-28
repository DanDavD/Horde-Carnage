#pragma once

#include <array>

#include <raylib.h>

namespace horde {

/// A still of the fight, blurred, for behind the pause menu: the fight held far away, out of focus.
///
/// The scene is drawn once into a texture at the game's resolution, then halved three times with smooth
/// filtering, and the smallest copy is stretched back over the screen. Halving and stretching is a blur
/// without a shader, so it works the same in a browser.
struct Backdrop {
    RenderTexture2D full{};
    std::array<RenderTexture2D, 3> halves{};
    bool ready = false; ///< Whether it holds the current pause's still.
};

/// Makes the textures. Needs an open window.
[[nodiscard]] Backdrop LoadBackdrop();

/// Frees them. Needs the window still open.
void UnloadBackdrop(Backdrop& backdrop);

/// Starts drawing the still: everything drawn until `EndBackdropCapture` goes into it, in game pixels.
void BeginBackdropCapture(Backdrop& backdrop);

/// Ends the still and blurs it.
void EndBackdropCapture(Backdrop& backdrop);

/// The blurred still over the whole game area, darkened, with a smaller copy over it that recedes as
/// `seconds` grows past the first fifth of a second, so the fight seems to step back behind the menu.
void DrawBackdrop(const Backdrop& backdrop, float seconds);

} // namespace horde
