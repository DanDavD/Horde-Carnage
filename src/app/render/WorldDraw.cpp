#include "app/render/WorldDraw.h"

#include <algorithm>
#include <cmath>

#include "app/RaylibConvert.h"
#include "app/render/ArenaDraw.h"
#include "app/render/BossDraw.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/ParticleDraw.h"
#include "app/render/PlayerDraw.h"
#include "app/render/Shapes.h"
#include "app/render/View.h"
#include "core/LaserSight.h"

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
constexpr float kBulletStreak = 16.0f;
constexpr float kBulletWidth = 4.0f;
constexpr float kLaserGlowWidth = 5.0f;
constexpr float kLaserBeamWidth = 2.0f;
constexpr float kLaserCoreWidth = 1.0f;

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

// A thin crimson sight from the gun barrel along the aim line. It terminates on the nearest living target
// or wall, and in open space spans the exact range of the equipped gun (750 on pistol, 600 on SMG) with a
// distinct range pip at its tip so the player reads weapon reach directly on the floor.
void DrawLaserSight(const LaserSight& sight, const Art& art) {
    const Vec2 delta = sight.end - sight.start;
    if (LengthSquared(delta) <= 0.25f) {
        return;
    }
    const bool stopped = sight.hitsTarget || sight.hitsWall;

    BeginBlendMode(BLEND_ADDITIVE);
    DrawLineEx(ToRaylib(sight.start), ToRaylib(sight.end), kLaserGlowWidth, Fade(kLaserSight, 0.16f));
    if (stopped) {
        DrawGlow(art, sight.end, sight.hitsTarget ? 16.0f : 11.0f,
                 Fade(kLaserSight, sight.hitsTarget ? 0.55f : 0.32f));
    } else {
        DrawGlow(art, sight.end, 8.0f, Fade(kLaserSight, 0.28f));
    }
    EndBlendMode();

    DrawLineEx(ToRaylib(sight.start), ToRaylib(sight.end), kLaserBeamWidth, Fade(kLaserSight, 0.68f));
    DrawLineEx(ToRaylib(sight.start), ToRaylib(sight.end), kLaserCoreWidth, Fade(kLaserCore, 0.5f));
    if (stopped) {
        const float dotRadius = sight.hitsTarget ? 3.2f : 2.3f;
        DrawCircleV(ToRaylib(sight.end), dotRadius, kLaserSight);
        DrawCircleV(ToRaylib(sight.end), dotRadius * 0.5f, kLaserCore);
    } else {
        DrawCircleV(ToRaylib(sight.end), 2.0f, Fade(kLaserSight, 0.85f));
        DrawCircleV(ToRaylib(sight.end), 1.0f, Fade(kLaserCore, 0.75f));
    }
}

void DrawBullet(const Projectile& bullet, float alpha) {
    const Vec2 head = Lerp(bullet.motion.previousPosition, bullet.motion.position, alpha);
    const Vec2 direction = Normalized(bullet.motion.velocity);
    const float travelled = std::max(Dot(head - bullet.origin, direction), 0.0f);
    const Vec2 tail = head - direction * std::min(kBulletStreak, travelled);
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

void DrawWorld(const World& world, const ParticlePool& particles, float alpha, const Art& art,
               float bossSinceDied, bool showAimSight, std::optional<Vec2> aimDirection) {
    const Player& player = world.player;
    const Vec2 playerPosition = Lerp(player.motion.previousPosition, player.motion.position, alpha);
    const Vec2 bossPosition = Lerp(world.boss.motion.previousPosition, world.boss.motion.position, alpha);
    const bool bossAirborne = IsAirborne(world.boss);

    DrawFloor(world.arena, art);
    DrawWalls(world.arena);
    // Shadows go on the floor before the light, so they sink into the dark with it. A boss in the air has
    // the Leap's own shadow instead.
    if (!bossAirborne) {
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
    DrawFloorParticles(particles);

    // On the floor, under every body, and over the dark, so a warning reads wherever it is.
    DrawBossTelegraphs(world.boss, world.arena);
    for (const Shockwave& wave : world.shockwaves) {
        DrawShockwave(wave);
    }
    for (const SpawnMarker& marker : world.spawnMarkers) {
        DrawSpawnMarker(marker);
    }
    DrawUnderbodyParticles(particles, art);

    // The boss goes under its children, so a dead boss lies on the floor. In the air it goes over
    // everything, the player included.
    if (!bossAirborne) {
        DrawBoss(world.boss, playerPosition, world.elapsedSeconds, alpha, art, bossSinceDied);
    }
    for (const Enemy& enemy : world.enemies) {
        DrawHairball(enemy, playerPosition, world.boss.summon.spitWarningSeconds, world.elapsedSeconds, alpha,
                     art);
    }
    if (showAimSight) {
        if (const std::optional<LaserSight> sight = TraceWorldLaserSight(world, alpha, aimDirection)) {
            DrawLaserSight(*sight, art);
        }
    }
    for (const Projectile& bullet : world.bullets) {
        DrawBullet(bullet, alpha);
    }
    for (const Projectile& strand : world.hostileProjectiles) {
        DrawHairStrand(strand, alpha);
    }
    // The player is drawn after every body on the floor, so it is never hidden under a crowd.
    DrawPlayer(player, world.elapsedSeconds, alpha, aimDirection);
    DrawOverbodyParticles(particles, art);
    if (bossAirborne) {
        DrawBoss(world.boss, playerPosition, world.elapsedSeconds, alpha, art, bossSinceDied);
    }
}

} // namespace horde
