#include "core/Menu.h"

namespace horde {

Menu MakeMenu(int count) {
    return Menu{.count = count > 0 ? count : 0};
}

MenuResult StepMenu(Menu& menu, const MenuInput& input) {
    if (menu.count <= 0) {
        return {};
    }
    if (input.back) {
        return MenuResult{MenuAction::Back, menu.selected};
    }
    const int before = menu.selected;
    if (input.hovered && *input.hovered >= 0 && *input.hovered < menu.count) {
        menu.selected = *input.hovered;
    } else if (input.move != 0) {
        menu.selected = ((menu.selected + input.move) % menu.count + menu.count) % menu.count;
    }
    if (input.confirm) {
        return MenuResult{MenuAction::Chosen, menu.selected};
    }
    return MenuResult{menu.selected != before ? MenuAction::Moved : MenuAction::None, menu.selected};
}

} // namespace horde
