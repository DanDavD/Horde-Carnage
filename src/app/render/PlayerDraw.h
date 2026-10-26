#pragma once

#include "core/Player.h"

namespace horde {

/// The player as a hunter in a tricorn hat, seen from above and interpolated by `alpha`: a torn cape that
/// trails behind the way they move and sways with `seconds`, a coat and a bandolier, the gun in hand held in
/// both hands toward the aim and the other one carried, the dash trail, the drink ring, and the blink while
/// hits cannot land.
void DrawPlayer(const Player& player, float seconds, float alpha);

} // namespace horde
