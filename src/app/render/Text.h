#pragma once

#include <raylib.h>

#include "core/Vec2.h"

namespace horde {

/// Letter spacing as a share of the font size. Cinzel is drawn from Roman capitals and reads best with a
/// little air between letters; titles get more.
inline constexpr float kTextTracking = 0.06f;
inline constexpr float kTitleTracking = 0.14f;

/// `text` in `font` at `size` game pixels, with its top left corner at `at`.
void DrawGameText(const Font& font, const char* text, Vec2 at, float size, Color color,
                  float tracking = kTextTracking);

/// `text` centred on `centerX`, with its top at `y`.
void DrawGameTextCentered(const Font& font, const char* text, float centerX, float y, float size, Color color,
                          float tracking = kTextTracking);

/// `text` ending at `rightX`, with its top at `y`.
void DrawGameTextRight(const Font& font, const char* text, float rightX, float y, float size, Color color,
                       float tracking = kTextTracking);

/// How wide `text` is in game pixels.
[[nodiscard]] float GameTextWidth(const Font& font, const char* text, float size, float tracking = kTextTracking);

} // namespace horde
