#pragma once

#include <raylib.h>

#include "core/Enemy.h"
#include "core/Vec2.h"

namespace horde {

// Hairballs and the boss: a round body with short tufts around it, and two eyes that look at the player.
// Sizes are fractions of the body radius, so the boss is the same drawing, scaled up.
inline constexpr float kTuftLength = 0.35f;

// How a hairy body is drawn. `flash` is how far a hit turns it toward white: all the way for a hairball,
// which dies in a few hits, but only part of the way for the boss, which the SMG hits ten times a second
// and which would otherwise stay white for the whole fight.
struct HairStyle {
    Color fill;
    Color edge;
    Color eye;
    int tufts;
    float tuftWidth;
    float outline;
    float flash;
};

/// `from` moved toward `to` by `amount`, from 0 to 1, channel by channel.
[[nodiscard]] Color Mix(Color from, Color to, float amount);

/// A round, hairy body with tufts around it and two eyes that look at `lookAt`: the hairballs and the boss
/// are the same drawing at different sizes. `flashing` turns it toward white after a hit.
void DrawHairBody(Vec2 position, float radius, Vec2 lookAt, const HairStyle& style, bool flashing);

/// A hairball, interpolated by `alpha`, swelling with red eyes for `spitWarningSeconds` before it spits.
void DrawHairball(const Enemy& enemy, Vec2 playerPosition, float spitWarningSeconds, float alpha);

} // namespace horde
