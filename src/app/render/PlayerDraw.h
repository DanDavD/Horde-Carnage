#pragma once

#include "core/Player.h"

namespace horde {

/// The player, interpolated by `alpha`: the body, the gun toward the aim, the dash trail, the drink ring,
/// and the blink while hits cannot land.
void DrawPlayer(const Player& player, float alpha);

} // namespace horde
