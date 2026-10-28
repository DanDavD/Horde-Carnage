#include "app/render/Backdrop.h"

#include <algorithm>

#include "app/render/Palette.h"
#include "app/render/View.h"

namespace horde {

namespace {

// How far the fight steps back, and how quickly.
constexpr float kRecededScale = 0.94f;
constexpr float kRecedeSeconds = 0.25f;
// How dark it is behind the menu: the edges darker than the middle, so the frame seems to fall away.
constexpr float kEdgeDark = 0.86f;
constexpr float kMiddleDark = 0.5f;

// A render texture's picture is stored upside down, so it is drawn with a negative height.
Rectangle Flipped(const RenderTexture2D& target) {
    return Rectangle{0.0f, 0.0f, static_cast<float>(target.texture.width), -static_cast<float>(target.texture.height)};
}

void DrawInto(const RenderTexture2D& from, const RenderTexture2D& into) {
    BeginTextureMode(into);
    ClearBackground(BLACK);
    DrawTexturePro(from.texture, Flipped(from),
                   Rectangle{0.0f, 0.0f, static_cast<float>(into.texture.width),
                             static_cast<float>(into.texture.height)},
                   Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    EndTextureMode();
}

void DrawStill(const RenderTexture2D& still, float scale, float dark) {
    const float width = kScreenWidth * scale;
    const float height = kScreenHeight * scale;
    const Rectangle dest{(kScreenWidth - width) * 0.5f, (kScreenHeight - height) * 0.5f, width, height};
    DrawTexturePro(still.texture, Flipped(still), dest, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    DrawRectangleRec(dest, Fade(kBackground, dark));
}

} // namespace

Backdrop LoadBackdrop() {
    Backdrop backdrop;
    backdrop.full = LoadRenderTexture(kScreenWidth, kScreenHeight);
    int width = kScreenWidth;
    int height = kScreenHeight;
    for (RenderTexture2D& half : backdrop.halves) {
        width /= 2;
        height /= 2;
        half = LoadRenderTexture(width, height);
        SetTextureFilter(half.texture, TEXTURE_FILTER_BILINEAR);
    }
    SetTextureFilter(backdrop.full.texture, TEXTURE_FILTER_BILINEAR);
    return backdrop;
}

void UnloadBackdrop(Backdrop& backdrop) {
    for (RenderTexture2D& half : backdrop.halves) {
        UnloadRenderTexture(half);
    }
    UnloadRenderTexture(backdrop.full);
    backdrop = Backdrop{};
}

void BeginBackdropCapture(Backdrop& backdrop) {
    BeginTextureMode(backdrop.full);
    ClearBackground(kBackground);
}

void EndBackdropCapture(Backdrop& backdrop) {
    EndTextureMode();
    // Halving one step at a time averages every pixel into the next copy; jumping straight to an eighth
    // would skip most of them and shimmer instead of blur.
    const RenderTexture2D* from = &backdrop.full;
    for (const RenderTexture2D& half : backdrop.halves) {
        DrawInto(*from, half);
        from = &half;
    }
    backdrop.ready = true;
}

void DrawBackdrop(const Backdrop& backdrop, float seconds) {
    const RenderTexture2D& blurred = backdrop.halves.back();
    const float t = std::clamp(seconds / kRecedeSeconds, 0.0f, 1.0f);
    const float eased = t * t * (3.0f - 2.0f * t);
    // The whole screen, darkest, so no black edge shows around the copy that steps back.
    DrawStill(blurred, 1.0f, kEdgeDark);
    DrawStill(blurred, 1.0f + (kRecededScale - 1.0f) * eased, kMiddleDark);
}

} // namespace horde
