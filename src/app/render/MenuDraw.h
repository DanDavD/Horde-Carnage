#pragma once

#include <optional>
#include <span>

#include <raylib.h>

#include "app/render/Art.h"
#include "core/Menu.h"
#include "core/Session.h"
#include "core/Settings.h"
#include "core/Vec2.h"

namespace horde {

/// Every screen with a menu. The title, the pause and the end screens belong to a state of the session; the
/// settings and the controls open over the title or the pause.
enum class MenuPage { None, Title, Pause, Dead, Victory, Settings, Controls };

// The items of the title and the pause menus, by position.
inline constexpr int kTitleBeginItem = 0;
inline constexpr int kTitleSettingsItem = 1;
inline constexpr int kPauseResumeItem = 0;
inline constexpr int kPauseSettingsItem = 1;

/// What a line of the settings screen does when it is chosen.
enum class SettingsRowKind { Value, Controls, Back };

/// One line of the settings screen.
struct SettingsRow {
    const char* label;
    SettingsRowKind kind;
    Setting setting = Setting::MasterVolume; ///< The value it changes, for a `Value` line.
};

/// The page shown in `state` when nothing is opened over it. None while playing.
[[nodiscard]] MenuPage PageFor(SessionState state);

/// How many items the menu of `page` has.
[[nodiscard]] int MenuItemCount(MenuPage page);

/// Whether the items of `page` sit in a row, where left and right move the selection instead of changing a
/// value.
[[nodiscard]] bool IsRowMenu(MenuPage page);

/// The lines of the settings screen. In a browser there is no display or frame rate to choose: the page
/// decides both.
[[nodiscard]] std::span<const SettingsRow> SettingsRows();

/// What the mouse points at in a menu: an item and, on a line of the settings, whether it is on the arrow
/// that lowers the value. A click anywhere else on the line raises it.
struct MenuTarget {
    int item = 0;
    bool lowerArrow = false;
};

/// The item under `point` (the mouse, in game pixels) on `page`, if there is one. The box around each item
/// is wider and taller than its letters, so a mouse does not have to land on a stroke.
[[nodiscard]] std::optional<MenuTarget> MenuTargetAt(MenuPage page, Vec2 point, const Art& art);

/// The items of the title, the pause, an end screen or the controls, in the serif of the titles and widely
/// spaced: grey, and the selected one in bone between a thin line and a small diamond on each side, which
/// slide out over the first tenth of a second (`sinceMoved`). Everything is faded by `shown`, from 0 to 1.
void DrawMenu(MenuPage page, const Menu& menu, float sinceMoved, float shown, const Art& art);

/// The darkened arena behind the title and the screens opened over it.
void DrawTitleShade();

/// More dark over whatever is behind the settings and the controls, which hold more text than a menu.
void DrawPageShade();

/// The title screen over the world drawn behind it: the game's name and the menu, all in the middle of the
/// screen, and the version at the bottom.
void DrawTitleScreen(const Menu& menu, float sinceMoved, const Art& art);

/// "PAUSED" and the pause menu, over the fight held behind it. Fades in over `seconds`.
void DrawPauseScreen(const Menu& menu, float sinceMoved, float seconds, const Art& art);

/// The settings: a name on the left of each line and its value on the right, with arrows on each side of
/// the selected value. The volume is a row of ten marks.
void DrawSettingsScreen(const Menu& menu, float sinceMoved, const Settings& settings, const Art& art);

/// What each key and button does, in a table, and Back.
void DrawControlsScreen(const Menu& menu, float sinceMoved, const Art& art);

} // namespace horde
