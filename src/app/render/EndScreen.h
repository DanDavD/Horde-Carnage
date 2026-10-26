#pragma once

#include "core/Session.h"

namespace horde {

/// "YOU DIED" or "VICTORY" over the frozen world, with the numbers of the fight, fading in over the restart
/// delay.
void DrawEndScreen(const Session& session);

} // namespace horde
