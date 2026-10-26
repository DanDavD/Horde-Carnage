#include "app/render/WorldDraw.h"

#include <algorithm>

#include "app/RaylibConvert.h"
#include "app/render/BossDraw.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/PlayerDraw.h"

namespace horde {

namespace {

constexpr float kGridSpacing = 80.0f;
constexpr float kWallThickness = 4.0f;
// A strand of hair is a short streak behind a round tip.
constexpr float kStrandTail = 12.0f;
// A summon mark is a dark pit with a red rim that starts this many times wider and closes in on it.
constexpr float kMarkerRadius = 18.0f;
constexpr float kMarkerRimStart = 2.5f;
// A bullet is drawn as a short streak behind its head, so its direction reads even in a still frame.
constexpr float kBulletStreak = 14.0f;
constexpr float kBulletWidth = 4.0f;

void DrawArena(const Arena& arena) {
    const Vec2 size = arena.max - arena.min;
    DrawRectangleV(ToRaylib(arena.min), ToRaylib(size), kFloor);

    // The grid gives the eye something fixed to judge speed against.
    for (float x = arena.min.x + kGridSpacing; x < arena.max.x; x += kGridSpacing) {
        DrawLineV(ToRaylib({x, arena.min.y}), ToRaylib({x, arena.max.y}), kGrid);
    }
    for (float y = arena.min.y + kGridSpacing; y < arena.max.y; y += kGridSpacing) {
        DrawLineV(ToRaylib({arena.min.x, y}), ToRaylib({arena.max.x, y}), kGrid);
    }

    // raylib draws the outline inward, so the rectangle grows by the wall thickness. Otherwise a mover
    // confined to the arena would overlap the wall.
    const Rectangle walls{arena.min.x - kWallThickness, arena.min.y - kWallThickness,
                          size.x + 2.0f * kWallThickness, size.y + 2.0f * kWallThickness};
    DrawRectangleLinesEx(walls, kWallThickness, kWall);
}

// A dark pit the size of a hairball, with a red rim that closes in on it: when the rim reaches the pit, the
// hairball comes out.
void DrawSpawnMarker(const SpawnMarker& marker) {
    const float left = std::clamp(marker.secondsLeft / marker.totalSeconds, 0.0f, 1.0f);
    const Vector2 center = ToRaylib(marker.position);
    DrawCircleV(center, kMarkerRadius, Fade(BLACK, 0.45f));
    const float rim = kMarkerRadius * (1.0f + (kMarkerRimStart - 1.0f) * left);
    DrawRing(center, rim - 2.0f, rim, 0.0f, 360.0f, 32, Fade(kBlood, 0.9f - 0.5f * left));
}

// The band that hurts, fading as the ring spreads out. A wave that already hit the player is drawn faint,
// so it is clear it cannot hurt again.
void DrawShockwave(const Shockwave& wave) {
    const ShockwaveStats& stats = wave.stats;
    const float left = 1.0f - std::min(wave.radius / stats.reach, 1.0f);
    const float inner = std::max(wave.radius - stats.thickness, 0.0f);
    DrawRing(ToRaylib(wave.center), inner, wave.radius, 0.0f, 360.0f, 96,
             Fade(kShockwave, (wave.spent ? 0.25f : 0.9f) * (0.3f + 0.7f * left)));
}

void DrawBullet(const Projectile& bullet, float alpha) {
    const Vec2 head = Lerp(bullet.motion.previousPosition, bullet.motion.position, alpha);
    const Vec2 tail = head - Normalized(bullet.motion.velocity) * kBulletStreak;
    DrawLineEx(ToRaylib(tail), ToRaylib(head), kBulletWidth, kBullet);
    DrawCircleV(ToRaylib(head), bullet.radius, kBullet);
}

// A dark streak behind a pale tip, as wide as the strand's hit circle, so what the player sees is what can
// hurt them.
void DrawHairStrand(const Projectile& strand, float alpha) {
    const Vec2 head = Lerp(strand.motion.previousPosition, strand.motion.position, alpha);
    const Vec2 tail = head - Normalized(strand.motion.velocity) * kStrandTail;
    DrawLineEx(ToRaylib(tail), ToRaylib(head), strand.radius * 1.2f, kBossEdge);
    DrawCircleV(ToRaylib(head), strand.radius, kBossEdge);
    DrawCircleV(ToRaylib(head), strand.radius - 3.0f, kStrand);
}

} // namespace

void DrawWorld(const World& world, float alpha) {
    const Player& player = world.player;
    const Vec2 playerPosition = Lerp(player.motion.previousPosition, player.motion.position, alpha);

    DrawArena(world.arena);
    // On the floor, under every body.
    DrawBossTelegraphs(world.boss, world.arena);
    for (const Shockwave& wave : world.shockwaves) {
        DrawShockwave(wave);
    }
    for (const SpawnMarker& marker : world.spawnMarkers) {
        DrawSpawnMarker(marker);
    }

    // The boss goes under its children, so a dead boss lies on the floor. In the air it goes over
    // everything, the player included.
    const bool bossAirborne = IsAirborne(world.boss);
    if (!bossAirborne) {
        DrawBoss(world.boss, playerPosition, alpha);
    }
    for (const Enemy& enemy : world.enemies) {
        DrawHairball(enemy, playerPosition, world.boss.summon.spitWarningSeconds, alpha);
    }
    for (const Projectile& bullet : world.bullets) {
        DrawBullet(bullet, alpha);
    }
    for (const Projectile& strand : world.hostileProjectiles) {
        DrawHairStrand(strand, alpha);
    }
    // The player is drawn after every body on the floor, so it is never hidden under a crowd.
    DrawPlayer(player, alpha);
    if (bossAirborne) {
        DrawBoss(world.boss, playerPosition, alpha);
    }
}

} // namespace horde
