#include <doctest/doctest.h>

#include "core/Menu.h"

using horde::Menu;
using horde::MenuAction;
using horde::MenuInput;
using horde::MenuResult;

TEST_CASE("a new menu selects its first item, and nothing happens without input") {
    Menu menu = horde::MakeMenu(3);
    CHECK(menu.selected == 0);
    const MenuResult result = horde::StepMenu(menu, MenuInput{});
    CHECK(result.action == MenuAction::None);
    CHECK(menu.selected == 0);
}

TEST_CASE("moving goes item by item and wraps round at both ends") {
    Menu menu = horde::MakeMenu(3);
    CHECK(horde::StepMenu(menu, MenuInput{.move = 1}).action == MenuAction::Moved);
    CHECK(menu.selected == 1);
    horde::StepMenu(menu, MenuInput{.move = 1});
    horde::StepMenu(menu, MenuInput{.move = 1});
    CHECK(menu.selected == 0);
    horde::StepMenu(menu, MenuInput{.move = -1});
    CHECK(menu.selected == 2);
}

TEST_CASE("the mouse selects the item it moves onto, over a key in the same frame") {
    Menu menu = horde::MakeMenu(4);
    const MenuResult result = horde::StepMenu(menu, MenuInput{.move = 1, .hovered = 3});
    CHECK(result.action == MenuAction::Moved);
    CHECK(menu.selected == 3);
    // Hovering the item already selected is not a move, and an item that does not exist is ignored.
    CHECK(horde::StepMenu(menu, MenuInput{.hovered = 3}).action == MenuAction::None);
    CHECK(horde::StepMenu(menu, MenuInput{.hovered = 9}).action == MenuAction::None);
    CHECK(menu.selected == 3);
}

TEST_CASE("confirming chooses the selected item, and a click chooses the item under the mouse") {
    Menu menu = horde::MakeMenu(3);
    horde::StepMenu(menu, MenuInput{.move = 1});
    MenuResult result = horde::StepMenu(menu, MenuInput{.confirm = true});
    CHECK(result.action == MenuAction::Chosen);
    CHECK(result.item == 1);

    result = horde::StepMenu(menu, MenuInput{.hovered = 2, .confirm = true});
    CHECK(result.action == MenuAction::Chosen);
    CHECK(result.item == 2);
}

TEST_CASE("an adjust asks to change the selected item's value, on the item the mouse points at") {
    Menu menu = horde::MakeMenu(3);
    MenuResult result = horde::StepMenu(menu, MenuInput{.adjust = -3});
    CHECK(result.action == MenuAction::Adjusted);
    CHECK(result.item == 0);
    CHECK(result.adjust == -1);

    result = horde::StepMenu(menu, MenuInput{.adjust = 1, .hovered = 2});
    CHECK(result.action == MenuAction::Adjusted);
    CHECK(result.item == 2);
    CHECK(result.adjust == 1);
    CHECK(menu.selected == 2);

    // A confirm in the same frame wins: it is a choice, not a change of value.
    CHECK(horde::StepMenu(menu, MenuInput{.adjust = 1, .confirm = true}).action == MenuAction::Chosen);
}

TEST_CASE("going back wins over everything else in the same frame, and keeps the selection") {
    Menu menu = horde::MakeMenu(3);
    horde::StepMenu(menu, MenuInput{.move = 1});
    const MenuResult result =
        horde::StepMenu(menu, MenuInput{.move = 1, .adjust = 1, .hovered = 0, .confirm = true, .back = true});
    CHECK(result.action == MenuAction::Back);
    CHECK(menu.selected == 1);
}

TEST_CASE("an empty menu ignores everything") {
    Menu menu = horde::MakeMenu(0);
    CHECK(horde::StepMenu(menu, MenuInput{.move = 1, .confirm = true}).action == MenuAction::None);
    CHECK(horde::MakeMenu(-2).count == 0);
}
