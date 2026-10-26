#include "app/render/Art.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "core/Random.h"

namespace horde {

// Compiled into the executable by cmake/EmbedFile.cmake.
extern const unsigned char kCinzelRegular[];
extern const std::size_t kCinzelRegularSize;
extern const unsigned char kCinzelBold[];
extern const std::size_t kCinzelBoldSize;

namespace {

// The floor is the same in every fight, so the arena is a place the player learns.
constexpr std::uint64_t kFloorSeed = 1031;
constexpr std::uint64_t kGrainSeed = 77;
// Fonts are drawn scaled with the rest of the game, so they are rasterised large and shrunk on the GPU:
// sharp at 1080p and above instead of blurred up from a small size.
constexpr int kTitleFontPixels = 160;
constexpr int kUiFontPixels = 64;
// The grain tiles, so it has to be a power of two for WebGL 1 to repeat it.
constexpr int kGrainSize = 256;
// How dark the darkest grain gets the stone it is multiplied over, from 0 to 1.
constexpr float kGrainDepth = 0.2f;
constexpr int kGlowSize = 128;
constexpr int kLightSize = 256;
// The share of the torch's reach that is fully lit, before the light starts to fade.
constexpr float kLitShare = 0.18f;
constexpr int kVignetteWidth = 320;
constexpr int kVignetteHeight = 180;

float SmoothStep(float from, float to, float x) {
    const float t = std::clamp((x - from) / (to - from), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Font LoadEmbeddedFont(const unsigned char* data, std::size_t size, int pixels) {
    Font font = LoadFontFromMemory(".ttf", data, static_cast<int>(size), pixels, nullptr, 0);
    // Mipmaps keep small text smooth when the large glyphs are shrunk.
    GenTextureMipmaps(&font.texture);
    SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
    return font;
}

// A value at (x, y) from a grid of random values that wraps at its edges, smoothly blended between the four
// nearest, so the noise has no seam where the texture repeats.
template <std::size_t N>
float Lattice(const std::array<float, N * N>& values, int x, int y) {
    const float cell = static_cast<float>(kGrainSize) / static_cast<float>(N);
    const float fx = static_cast<float>(x) / cell;
    const float fy = static_cast<float>(y) / cell;
    const int x0 = static_cast<int>(fx);
    const int y0 = static_cast<int>(fy);
    const float tx = SmoothStep(0.0f, 1.0f, fx - static_cast<float>(x0));
    const float ty = SmoothStep(0.0f, 1.0f, fy - static_cast<float>(y0));
    const auto at = [&values](int i, int j) {
        const auto wrap = [](int k) { return static_cast<std::size_t>(k % static_cast<int>(N)); };
        return values[wrap(j) * N + wrap(i)];
    };
    const float top = at(x0, y0) + (at(x0 + 1, y0) - at(x0, y0)) * tx;
    const float bottom = at(x0, y0 + 1) + (at(x0 + 1, y0 + 1) - at(x0, y0 + 1)) * tx;
    return top + (bottom - top) * ty;
}

template <std::size_t N>
std::array<float, N * N> RandomValues(Random& random) {
    std::array<float, N * N> values{};
    for (float& value : values) {
        value = NextFloat(random);
    }
    return values;
}

// Blotches the size of a hand, finer mottling, and a speckle of single texels, from light to dark grey.
Texture2D MakeGrain() {
    Random random = SeedRandom(kGrainSeed);
    const auto coarse = RandomValues<8>(random);
    const auto fine = RandomValues<32>(random);
    Image image = GenImageColor(kGrainSize, kGrainSize, WHITE);
    Color* pixels = static_cast<Color*>(image.data);
    for (int y = 0; y < kGrainSize; ++y) {
        for (int x = 0; x < kGrainSize; ++x) {
            const float noise =
                0.55f * Lattice<8>(coarse, x, y) + 0.3f * Lattice<32>(fine, x, y) + 0.15f * NextFloat(random);
            const auto grey = static_cast<unsigned char>(255.0f * (1.0f - kGrainDepth * noise));
            pixels[y * kGrainSize + x] = Color{grey, grey, grey, 255};
        }
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

// White everywhere, with an alpha that `alphaAt` gives for each texel's distance from the middle, from 0 at
// the middle to 1 at the middle of an edge.
template <typename AlphaAt>
Texture2D MakeRadial(int width, int height, AlphaAt alphaAt) {
    Image image = GenImageColor(width, height, BLANK);
    Color* pixels = static_cast<Color*>(image.data);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f) / static_cast<float>(width) * 2.0f - 1.0f;
            const float dy = (static_cast<float>(y) + 0.5f) / static_cast<float>(height) * 2.0f - 1.0f;
            const float alpha = std::clamp(alphaAt(std::sqrt(dx * dx + dy * dy)), 0.0f, 1.0f);
            pixels[y * width + x] = Color{255, 255, 255, static_cast<unsigned char>(255.0f * alpha)};
        }
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

} // namespace

Art LoadArt(const Arena& arena) {
    Art art;
    art.titleFont = LoadEmbeddedFont(kCinzelRegular, kCinzelRegularSize, kTitleFontPixels);
    art.uiFont = LoadEmbeddedFont(kCinzelBold, kCinzelBoldSize, kUiFontPixels);
    art.grain = MakeGrain();
    art.glow = MakeRadial(kGlowSize, kGlowSize, [](float d) { return 1.0f - SmoothStep(0.0f, 1.0f, d); });
    // Opaque well before the edge, so the light's square meets the dark around it without a seam.
    art.light = MakeRadial(kLightSize, kLightSize, [](float d) { return SmoothStep(kLitShare, 0.95f, d); });
    art.vignette = MakeRadial(kVignetteWidth, kVignetteHeight, [](float d) { return SmoothStep(0.55f, 1.45f, d); });
    art.floor = LayStoneFloor(arena, kFloorSeed);
    return art;
}

void UnloadArt(Art& art) {
    UnloadFont(art.titleFont);
    UnloadFont(art.uiFont);
    UnloadTexture(art.grain);
    UnloadTexture(art.glow);
    UnloadTexture(art.light);
    UnloadTexture(art.vignette);
    art = Art{};
}

void DrawGlow(const Art& art, Vec2 center, float radius, Color color) {
    const Rectangle source{0.0f, 0.0f, static_cast<float>(art.glow.width), static_cast<float>(art.glow.height)};
    const Rectangle dest{center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f};
    DrawTexturePro(art.glow, source, dest, Vector2{0.0f, 0.0f}, 0.0f, color);
}

} // namespace horde
