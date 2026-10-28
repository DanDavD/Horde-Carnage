#pragma once

#include <optional>

namespace horde {

/// A list of choices and the one that is selected: the title screen, the pause menu, the end screens.
struct Menu {
    int count = 0;
    int selected = 0;
};

/// What the player did to a menu in one frame, from whatever they used: keys, a gamepad or the mouse.
struct MenuInput {
    int move = 0;               ///< -1 to the previous item, 1 to the next, 0 to stay.
    std::optional<int> hovered; ///< The item the mouse moved onto this frame, if it moved onto one.
    bool confirm = false;       ///< Enter, Space, the gamepad's A, or a click on the hovered item.
    bool back = false;          ///< Esc, Backspace or the gamepad's B.
};

/// What a menu did with that input.
enum class MenuAction {
    None,
    Moved,  ///< The selection changed.
    Chosen, ///< The selected item was confirmed.
    Back,   ///< The player asked to leave the menu.
};

struct MenuResult {
    MenuAction action = MenuAction::None;
    int item = 0; ///< The selected item after the input.
};

/// A menu with `count` items and the first one selected.
[[nodiscard]] Menu MakeMenu(int count);

/// Applies one frame of input. The mouse wins over a key in the same frame, since it points at an item.
/// Moving past either end wraps round, so the last item is one press away from the first. A confirm
/// chooses the selected item; going back wins over everything, so a menu can always be left.
MenuResult StepMenu(Menu& menu, const MenuInput& input);

} // namespace horde
