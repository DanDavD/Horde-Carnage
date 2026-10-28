#pragma once

#include "app/render/Art.h"
#include "core/Menu.h"
#include "core/Session.h"

namespace horde {

/// "YOU DIED" or "VICTORY" over the frozen world, with the numbers of the fight and the row of choices
/// (`menu`), fading in over the restart delay. The choices stay dim until a choice can be made.
void DrawEndScreen(const Session& session, const Menu& menu, float sinceMoved, const Art& art);

} // namespace horde
