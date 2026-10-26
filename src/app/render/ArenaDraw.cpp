#include "app/render/ArenaDraw.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

#include "app/RaylibConvert.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/View.h"

namespace horde {

namespace {

// The floor. Slabs are inset by half a joint on every side, so the mortar shows between them. Their tone
// moves the stone's colour up or down by this much per channel, and a bevel of this width is lit on the top
// and left edges and shaded on the others, as if the light came from the top left, so each slab stands a
// little proud of its joints.
constexpr float kJoint = 4.0f;
constexpr float kStoneToneRange = 12.0f;
constexpr float kBevel = 2.0f;
constexpr float kBevelLight = 0.12f;
constexpr float kBevelShade = 0.3f;
constexpr float kCrackWidth = 1.5f;
constexpr float kStainStrength = 0.5f;
// The walls fill everything the camera can show past the arena (`FollowCameraStats::wallMargin`), in two
// courses of blocks laid with a half-block offset, like a real wall seen from above.
constexpr float kWallDepth = 64.0f;
constexpr int kWallCourses = 2;
constexpr float kWallBlock = 96.0f;
constexpr float kWallJoint = 3.0f;
constexpr float kWallToneRange = 5.0f;
constexpr float kWallLipWidth = 3.0f;
// The floor darkens over this distance from the foot of every wall.
constexpr float kWallShadowReach = 40.0f;
constexpr float kWallShadowStrength = 0.5f;
// Shadows lean away from the torch, more the further the body stands from it, up to this many radii. They
// are a little longer than the body along the lean and as wide as it across.
constexpr float kShadowLean = 0.08f;
constexpr float kShadowMaxLean = 0.6f;
constexpr float kShadowStrength = 0.55f;
// The torch. Its light reaches this far before everything is as dark as `kDarknessStrength`, its warmth
// reaches this far, and both breathe by a few percent, like a flame.
constexpr float kTorchReach = 470.0f;
constexpr float kDarknessStrength = 0.74f;
constexpr float kWarmthReach = 260.0f;
constexpr float kWarmth = 0.17f;
constexpr float kFlicker = 0.025f;
constexpr float kVignetteStrength = 0.6f;

Color Shade(Color color, float amount) {
    const auto channel = [amount](unsigned char value) {
        return static_cast<unsigned char>(std::clamp(static_cast<float>(value) + amount, 0.0f, 255.0f));
    };
    return Color{channel(color.r), channel(color.g), channel(color.b), color.a};
}

void DrawStrip(Vec2 min, Vec2 max, Color color) {
    DrawRectangleV(ToRaylib(min), ToRaylib(max - min), color);
}

void DrawSlab(const Slab& slab) {
    const Vec2 min = slab.min + Vec2{kJoint, kJoint} * 0.5f;
    const Vec2 max = slab.max - Vec2{kJoint, kJoint} * 0.5f;
    const Color stone = Shade(kStone, slab.tone * kStoneToneRange);
    DrawStrip(min, max, stone);
    const Color lit = Mix(stone, WHITE, kBevelLight);
    const Color shaded = Mix(stone, BLACK, kBevelShade);
    DrawStrip(Vec2{min.x, max.y - kBevel}, max, shaded);
    DrawStrip(Vec2{max.x - kBevel, min.y}, max, shaded);
    DrawStrip(min, Vec2{max.x, min.y + kBevel}, lit);
    DrawStrip(min, Vec2{min.x + kBevel, max.y}, lit);
    if (slab.cracked) {
        DrawLineEx(ToRaylib(slab.crack[0]), ToRaylib(slab.crack[1]), kCrackWidth, kCrack);
        DrawLineEx(ToRaylib(slab.crack[1]), ToRaylib(slab.crack[2]), kCrackWidth, kCrack);
    }
}

// A tone from -1 to 1 that is the same for the same block every frame, without storing anything.
float BlockTone(int course, int block) {
    std::uint32_t hash = static_cast<std::uint32_t>(course) * 73856093u ^ static_cast<std::uint32_t>(block) * 19349663u;
    hash ^= hash >> 13;
    hash *= 0x5bd1e995u;
    hash ^= hash >> 15;
    return static_cast<float>(hash % 1001u) / 500.0f - 1.0f;
}

// One side of the wall, from `min` to `max`. `alongX` says which way its blocks run.
void DrawWallBand(Vec2 min, Vec2 max, bool alongX) {
    DrawStrip(min, max, kWallMortar);
    const float length = alongX ? max.x - min.x : max.y - min.y;
    const float depth = (alongX ? max.y - min.y : max.x - min.x) / static_cast<float>(kWallCourses);
    for (int course = 0; course < kWallCourses; ++course) {
        const float offset = course % 2 == 0 ? 0.0f : kWallBlock * 0.5f;
        int block = 0;
        for (float start = -offset; start < length; start += kWallBlock, ++block) {
            const float from = std::max(start, 0.0f) + kWallJoint * 0.5f;
            const float to = std::min(start + kWallBlock, length) - kWallJoint * 0.5f;
            const float across = static_cast<float>(course) * depth + kWallJoint * 0.5f;
            const float acrossEnd = across + depth - kWallJoint;
            const Color stone = Shade(kWallStone, BlockTone(course, block) * kWallToneRange);
            if (alongX) {
                DrawStrip(Vec2{min.x + from, min.y + across}, Vec2{min.x + to, min.y + acrossEnd}, stone);
            } else {
                DrawStrip(Vec2{min.x + across, min.y + from}, Vec2{min.x + acrossEnd, min.y + to}, stone);
            }
        }
    }
}

} // namespace

void DrawFloor(const Arena& arena, const Art& art) {
    const Vec2 size = arena.max - arena.min;
    DrawStrip(arena.min, arena.max, kMortar);
    for (const Slab& slab : art.floor.slabs) {
        DrawSlab(slab);
    }
    // The grain is multiplied over the stone, so the slabs read as rough rock instead of flat paint. It
    // repeats every 256 units, which the slabs and stains break up.
    BeginBlendMode(BLEND_MULTIPLIED);
    const Rectangle floor{arena.min.x, arena.min.y, size.x, size.y};
    DrawTexturePro(art.grain, Rectangle{0.0f, 0.0f, size.x, size.y}, floor, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    EndBlendMode();
    for (const Stain& stain : art.floor.stains) {
        DrawGlow(art, stain.center, stain.radius, Fade(kStain, kStainStrength));
    }
}

void DrawWalls(const Arena& arena) {
    const Vec2 size = arena.max - arena.min;
    const auto x = static_cast<int>(arena.min.x);
    const auto y = static_cast<int>(arena.min.y);
    const auto width = static_cast<int>(size.x);
    const auto height = static_cast<int>(size.y);
    const auto reach = static_cast<int>(kWallShadowReach);
    const Color dark = Fade(BLACK, kWallShadowStrength);
    const Color clear = Fade(BLACK, 0.0f);
    DrawRectangleGradientV(x, y, width, reach, dark, clear);
    DrawRectangleGradientV(x, y + height - reach, width, reach, clear, dark);
    DrawRectangleGradientH(x, y, reach, height, dark, clear);
    DrawRectangleGradientH(x + width - reach, y, reach, height, clear, dark);

    const float d = kWallDepth;
    DrawWallBand(Vec2{arena.min.x - d, arena.min.y - d}, Vec2{arena.max.x + d, arena.min.y}, true);
    DrawWallBand(Vec2{arena.min.x - d, arena.max.y}, Vec2{arena.max.x + d, arena.max.y + d}, true);
    DrawWallBand(Vec2{arena.min.x - d, arena.min.y}, Vec2{arena.min.x, arena.max.y}, false);
    DrawWallBand(Vec2{arena.max.x, arena.min.y}, Vec2{arena.max.x + d, arena.max.y}, false);
    // raylib draws the outline inward, so the rectangle grows by the lip: it sits on the wall, and a body
    // confined to the arena touches it without overlapping it.
    const Rectangle lip{arena.min.x - kWallLipWidth, arena.min.y - kWallLipWidth, size.x + 2.0f * kWallLipWidth,
                        size.y + 2.0f * kWallLipWidth};
    DrawRectangleLinesEx(lip, kWallLipWidth, kWallLip);
}

void DrawBodyShadow(Vec2 position, float radius, Vec2 light, const Art& art) {
    const Vec2 away = position - light;
    const float distance = Length(away);
    // Under the torch itself, the player's own shadow falls straight down a little.
    const Vec2 direction = distance > 1.0f ? away * (1.0f / distance) : Vec2{0.0f, 1.0f};
    const float lean = std::min(distance * kShadowLean, radius * kShadowMaxLean) + radius * 0.1f;
    const Vec2 center = position + direction * lean;
    const float length = radius * 2.3f + lean;
    const float width = radius * 2.1f;
    const Rectangle source{0.0f, 0.0f, static_cast<float>(art.glow.width), static_cast<float>(art.glow.height)};
    const Rectangle dest{center.x, center.y, length, width};
    const float degrees = std::atan2(direction.y, direction.x) * 180.0f / std::numbers::pi_v<float>;
    DrawTexturePro(art.glow, source, dest, Vector2{length * 0.5f, width * 0.5f}, degrees,
                   Fade(BLACK, kShadowStrength));
}

void DrawTorchLight(Vec2 torch, Vec2 viewMin, Vec2 viewMax, float seconds, const Art& art) {
    const float flicker = 1.0f + kFlicker * (0.6f * std::sin(seconds * 7.3f) + 0.4f * std::sin(seconds * 13.7f));
    const float reach = kTorchReach * flicker;
    const Color dark = Fade(kDarkness, kDarknessStrength);

    // The light texture covers a square around the torch; the rest of the view is fully dark, in up to four
    // rectangles that meet the square's edges exactly.
    const Vec2 low = torch - Vec2{reach, reach};
    const Vec2 high = torch + Vec2{reach, reach};
    const Rectangle source{0.0f, 0.0f, static_cast<float>(art.light.width), static_cast<float>(art.light.height)};
    DrawTexturePro(art.light, source, Rectangle{low.x, low.y, high.x - low.x, high.y - low.y}, Vector2{0.0f, 0.0f},
                   0.0f, dark);
    if (low.y > viewMin.y) {
        DrawStrip(viewMin, Vec2{viewMax.x, low.y}, dark);
    }
    if (high.y < viewMax.y) {
        DrawStrip(Vec2{viewMin.x, high.y}, viewMax, dark);
    }
    const float top = std::max(low.y, viewMin.y);
    const float bottom = std::min(high.y, viewMax.y);
    if (low.x > viewMin.x && bottom > top) {
        DrawStrip(Vec2{viewMin.x, top}, Vec2{low.x, bottom}, dark);
    }
    if (high.x < viewMax.x && bottom > top) {
        DrawStrip(Vec2{high.x, top}, Vec2{viewMax.x, bottom}, dark);
    }

    // The flame's warmth, added on top of the stone.
    BeginBlendMode(BLEND_ADDITIVE);
    DrawGlow(art, torch, kWarmthReach * flicker, Fade(kTorchLight, kWarmth));
    EndBlendMode();
}

void DrawVignette(const Art& art) {
    const Rectangle source{0.0f, 0.0f, static_cast<float>(art.vignette.width),
                           static_cast<float>(art.vignette.height)};
    const Rectangle dest{0.0f, 0.0f, static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight)};
    DrawTexturePro(art.vignette, source, dest, Vector2{0.0f, 0.0f}, 0.0f, Fade(BLACK, kVignetteStrength));
}

} // namespace horde
