#pragma once

#include <raylib.h>

#include "app/render/Art.h"
#include "core/Arena.h"
#include "core/Boss.h"
#include "core/Shield.h"
#include "core/Vec2.h"

namespace horde {

/// The warnings of the boss's attack on the floor, under every body: the Charge's path, the Hair Ring and
/// its gap, the Leap's shadow and the Volley's line.
void DrawBossTelegraphs(const Boss& boss, const Arena& arena);

/// The boss in its current pose, interpolated by `alpha`, with its shield and its stagger. It breathes and
/// pulses with `seconds`, the fight's clock, and looks at the player and is lit from their torch.
void DrawBoss(const Boss& boss, Vec2 playerPosition, float seconds, float alpha, const Art& art);

/// The colour of a kind of shield, on the boss and on its bar.
[[nodiscard]] Color ShieldColor(ShieldKind kind);

} // namespace horde
