#include "app/render/Text.h"

#include "app/RaylibConvert.h"

namespace horde {

void DrawGameText(const Font& font, const char* text, Vec2 at, float size, Color color, float tracking) {
    DrawTextEx(font, text, ToRaylib(at), size, size * tracking, color);
}

void DrawGameTextCentered(const Font& font, const char* text, float centerX, float y, float size, Color color,
                          float tracking) {
    const float width = GameTextWidth(font, text, size, tracking);
    DrawGameText(font, text, Vec2{centerX - width * 0.5f, y}, size, color, tracking);
}

void DrawGameTextRight(const Font& font, const char* text, float rightX, float y, float size, Color color,
                       float tracking) {
    DrawGameText(font, text, Vec2{rightX - GameTextWidth(font, text, size, tracking), y}, size, color, tracking);
}

float GameTextWidth(const Font& font, const char* text, float size, float tracking) {
    return MeasureTextEx(font, text, size, size * tracking).x;
}

} // namespace horde
