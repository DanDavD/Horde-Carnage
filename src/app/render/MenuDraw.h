#pragma once

#include <optional>
#include <span>

#include <raylib.h>

#include "app/render/Art.h"
#include "core/Menu.h"
#include "core/Session.h"
#include "core/Vec2.h"

namespace horde {

/// Where a menu's items sit on the screen, in game pixels: the middle of the first one, and the step to
/// each next one, down a column or along a row.
struct MenuLayout {
    Vec2 first;
    Vec2 step;
    float size = 22.0f; ///< The height of the letters.
};

/// The items of the menu shown in `state`: the title, the pause menu or an end screen. None while playing.
[[nodiscard]] std::span<const char* const> MenuLabelsFor(SessionState state);

/// Where the menu shown in `state` sits.
[[nodiscard]] MenuLayout MenuLayoutFor(SessionState state);

/// The item under `point` (the mouse, in game pixels), if there is one. The box around each item is wider
/// and taller than its letters, so a mouse does not have to land on a stroke.
[[nodiscard]] std::optional<int> MenuItemAt(const MenuLayout& layout, std::span<const char* const> labels,
                                           Vec2 point, const Art& art);

/// The items, in the serif of the titles and widely spaced: grey, and the selected one in bone between a
/// thin line and a small diamond on each side, which slide out over the first tenth of a second
/// (`sinceMoved`). Everything is faded by `shown`, from 0 to 1.
void DrawMenu(const MenuLayout& layout, std::span<const char* const> labels, const Menu& menu, float sinceMoved,
              float shown, const Art& art);

/// The title screen over the world drawn behind it: the darkened arena, the game's name and the menu, all
/// in the middle of the screen, and the version at the bottom.
void DrawTitleScreen(const Menu& menu, float sinceMoved, const Art& art);

/// "PAUSED" and the pause menu, over the fight held behind it. Fades in over `seconds`.
void DrawPauseScreen(const Menu& menu, float sinceMoved, float seconds, const Art& art);

} // namespace horde
