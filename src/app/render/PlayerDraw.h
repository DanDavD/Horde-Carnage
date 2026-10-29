#pragma once

#include <optional>

#include "core/Player.h"

namespace horde {

/// The player as a hunter in a tricorn hat, seen from above and interpolated by `alpha`: a torn cape that
/// trails behind the way they move and sways with `seconds`, a coat and a bandolier, the gun in hand held in
/// both hands toward the aim and the other one carried, the dash trail, the drink ring, and the blink while
/// hits cannot land. When `aimDirection` is set, the barrel turns with the live frame aim above 60 Hz.
void DrawPlayer(const Player& player, float seconds, float alpha,
                std::optional<Vec2> aimDirection = std::nullopt);

} // namespace horde
