#include "app/render/WorldDraw.h"

#include <algorithm>
#include <cmath>

#include "app/RaylibConvert.h"
#include "app/render/ArenaDraw.h"
#include "app/render/BossDraw.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/PlayerDraw.h"
#include "app/render/Shapes.h"
#include "app/render/View.h"

namespace horde {

namespace {

// A strand is a clump of hair with this many spikes, reaching this share of its radius past the ball, and
// spinning this many radians per unit it travels.
constexpr float kClumpSpikes = 7.0f;
constexpr float kClumpSpikeLength = 0.55f;
constexpr float kClumpSpinRate = 0.06f;
// A summon mark is a dark pit with a red rim that starts this many times wider and closes in on it.
constexpr float kMarkerRadius = 18.0f;
constexpr float kMarkerRimStart = 2.5f;
// A bullet is drawn as a short streak behind its head, so its direction reads even in a still frame.
constexpr float kBulletStreak = 14.0f;
constexpr float kBulletWidth = 4.0f;

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

// A small, spiky clump of the boss's hair, spinning as it flies, its spikes twitching, so it reads as torn
// from her and thrown. The solid ball is exactly the strand's hit circle, so what the player sees is what
// can hurt them; only the spikes reach past it.
void DrawHairStrand(const Projectile& strand, float alpha) {
    const Vec2 head = Lerp(strand.motion.previousPosition, strand.motion.position, alpha);
    // Spin and twitch come from where it is, so every clump moves on its own, with no clock to keep.
    const float spin = (head.x - head.y) * kClumpSpinRate;
    const float twitch = head.x + head.y;
    const auto spikes = [&](float extra) {
        return [&, extra](float angle) {
            const float turned = angle + spin;
            const float spike = std::pow(std::abs(std::cos(turned * kClumpSpikes * 0.5f)), 6.0f);
            const float which = std::floor(turned * kClumpSpikes / (2.0f * kPi));
            const float wobble = 0.75f + 0.25f * std::sin(twitch * 0.3f + which);
            return strand.radius * (1.0f + kClumpSpikeLength * spike * wobble) + extra;
        };
    };
    DrawRadialShape<48>(head, spikes(2.0f), kBossEdge);
    DrawRadialShape<48>(head, spikes(0.0f), kStrand);
    DrawCircleV(ToRaylib(head), strand.radius * 0.55f, Mix(kStrand, kBossEdge, 0.35f));
    DrawCircleV(ToRaylib(head + Vec2{-0.3f, -0.3f} * strand.radius), strand.radius * 0.25f, Fade(WHITE, 0.45f));
}

} // namespace

void DrawWorld(const World& world, float alpha, const Art& art) {
    const Player& player = world.player;
    const Vec2 playerPosition = Lerp(player.motion.previousPosition, player.motion.position, alpha);
    const bool bossAirborne = IsAirborne(world.boss);

    DrawFloor(world.arena, art);
    DrawWalls(world.arena);
    // Shadows go on the floor before the light, so they sink into the dark with it. A boss in the air has
    // the Leap's own shadow instead.
    if (!bossAirborne) {
        const Vec2 bossPosition = Lerp(world.boss.motion.previousPosition, world.boss.motion.position, alpha);
        DrawBodyShadow(bossPosition, world.boss.radius, playerPosition, art);
    }
    for (const Enemy& enemy : world.enemies) {
        const Vec2 position = Lerp(enemy.motion.previousPosition, enemy.motion.position, alpha);
        DrawBodyShadow(position, enemy.radius, playerPosition, art);
    }
    DrawBodyShadow(playerPosition, player.radius, playerPosition, art);
    const Vec2 viewCenter = Lerp(world.camera.previousCenter, world.camera.center, alpha);
    DrawTorchLight(playerPosition, viewCenter - kViewSize * 0.5f, viewCenter + kViewSize * 0.5f,
                   world.elapsedSeconds, art);

    // On the floor, under every body, and over the dark, so a warning reads wherever it is.
    DrawBossTelegraphs(world.boss, world.arena);
    for (const Shockwave& wave : world.shockwaves) {
        DrawShockwave(wave);
    }
    for (const SpawnMarker& marker : world.spawnMarkers) {
        DrawSpawnMarker(marker);
    }

    // The boss goes under its children, so a dead boss lies on the floor. In the air it goes over
    // everything, the player included.
    if (!bossAirborne) {
        DrawBoss(world.boss, playerPosition, world.elapsedSeconds, alpha, art);
    }
    for (const Enemy& enemy : world.enemies) {
        DrawHairball(enemy, playerPosition, world.boss.summon.spitWarningSeconds, world.elapsedSeconds, alpha,
                     art);
    }
    for (const Projectile& bullet : world.bullets) {
        DrawBullet(bullet, alpha);
    }
    for (const Projectile& strand : world.hostileProjectiles) {
        DrawHairStrand(strand, alpha);
    }
    // The player is drawn after every body on the floor, so it is never hidden under a crowd.
    DrawPlayer(player, world.elapsedSeconds, alpha);
    if (bossAirborne) {
        DrawBoss(world.boss, playerPosition, world.elapsedSeconds, alpha, art);
    }
}

} // namespace horde
