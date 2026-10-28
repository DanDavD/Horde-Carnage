#include "app/render/ParticleDraw.h"

#include <algorithm>
#include <cmath>

#include <raylib.h>

#include "app/RaylibConvert.h"
#include "app/render/HairBody.h"
#include "app/render/Palette.h"
#include "app/render/Shapes.h"

namespace horde {

namespace {

// Debris that lies on the floor stays solid until its last quarter, then sinks away instead of popping out.
constexpr float kDebrisFadeShare = 0.25f;

float DebrisFade(const Particle& particle) {
    return std::min(LifeLeft(particle) / kDebrisFadeShare, 1.0f);
}

Vec2 VisualPosition(const Particle& particle) {
    return {particle.position.x, particle.position.y - particle.height};
}

Color SparkColor(ParticleTint tint) {
    switch (tint) {
    case ParticleTint::Shell:
        return kShell;
    case ParticleTint::Barrier:
        return kBarrier;
    case ParticleTint::Plain:
    case ParticleTint::Boss:
    case ParticleTint::Hairball:
        return kMuzzleFlash;
    }
    return kMuzzleFlash;
}

Color ShardColor(ParticleTint tint) {
    return tint == ParticleTint::Barrier ? kBarrier : kShell;
}

Color BloodColor(ParticleTint tint) {
    return tint == ParticleTint::Plain ? kBlood : kDarkBlood;
}

struct HairColors {
    Color fill;
    Color edge;
    Color sheen;
};

HairColors HairPalette(ParticleTint tint) {
    if (tint == ParticleTint::Hairball) {
        return {.fill = kHairballFill, .edge = kHairballEdge, .sheen = kHairballSheen};
    }
    // Lifted slightly toward the sheen so fallen locks still read against dark stone away from the torch.
    return {.fill = Mix(kBossFill, kBossSheen, 0.38f), .edge = kBossEdge, .sheen = kBossSheen};
}

void DrawCasing(const Particle& particle, bool settled) {
    const float fade = DebrisFade(particle);
    const Vec2 center = VisualPosition(particle);
    const Vec2 along{std::cos(particle.angle), std::sin(particle.angle)};
    const Vec2 across{-along.y, along.x};
    const float halfLength = particle.size * 0.5f;
    const float width = particle.size * 0.46f;
    const Vec2 tail = center - along * halfLength;
    const Vec2 mouth = center + along * halfLength;

    DrawLineEx(ToRaylib(tail - along * 0.7f), ToRaylib(mouth + along * 0.7f), width + 1.5f,
               Fade(kCapeEdge, 0.9f * fade));
    const Color brass = settled ? Mix(kBrass, kLeather, 0.22f) : kBrass;
    DrawLineEx(ToRaylib(tail), ToRaylib(mouth), width, Fade(brass, fade));
    DrawLineEx(ToRaylib(tail + across * (width * 0.18f)), ToRaylib(mouth + across * (width * 0.18f)),
               width * 0.28f, Fade(kHitFlash, (settled ? 0.25f : 0.55f) * fade));
}

void DrawBloodSplat(const Particle& particle) {
    const float fade = DebrisFade(particle);
    const Vec2 along{std::cos(particle.angle), std::sin(particle.angle)};
    const Vec2 across{-along.y, along.x};
    const Color pool = Mix(BloodColor(particle.tint), kStain, 0.32f);
    DrawOrientedEllipse(particle.position, along, particle.size * 1.35f, particle.size * 0.95f,
                        Fade(pool, 0.85f * fade));
    const float side = (Hash(particle.seconds * 31.0f) - 0.5f) * particle.size * 1.4f;
    const Vec2 drop = particle.position + along * (particle.size * 1.1f) + across * side;
    DrawCircleV(ToRaylib(drop), particle.size * 0.42f, Fade(pool, 0.75f * fade));
}

void DrawBloodDrop(const Particle& particle) {
    const float fade = DebrisFade(particle);
    const Vec2 center = VisualPosition(particle);
    const Vec2 flight{particle.velocity.x, particle.velocity.y - particle.lift};
    const Vec2 along =
        LengthSquared(flight) > 1.0f ? Normalized(flight) : Vec2{std::cos(particle.angle), std::sin(particle.angle)};
    const Color color = BloodColor(particle.tint);
    DrawOrientedEllipse(center, along, particle.size * 1.2f + 0.8f, particle.size * 0.72f + 0.8f,
                        Fade(kBossEdge, 0.75f * fade));
    DrawOrientedEllipse(center, along, particle.size * 1.2f, particle.size * 0.72f, Fade(color, fade));
}

// A tapered, slightly curved lock of hair rather than a round clump, so shed locks never look like the
// spiky projectiles the boss and her children spit.
void DrawHairLock(const Particle& particle, bool settled) {
    const float fade = DebrisFade(particle) * (settled ? 0.88f : 1.0f);
    const Vec2 center = VisualPosition(particle);
    const Vec2 along{std::cos(particle.angle), std::sin(particle.angle)};
    const Vec2 across{-along.y, along.x};
    const float curl = (Hash(particle.seconds * 37.0f) - 0.5f) * 2.0f;
    const float half = particle.size * 0.55f;
    const float waist = particle.size * 0.22f;

    const Vec2 root = center - along * half;
    const Vec2 tip = center + along * half + across * (curl * particle.size * 0.35f);
    const Vec2 fork = center + along * (half * 0.78f) - across * (curl * particle.size * 0.22f);
    const Vec2 left = root + across * waist;
    const Vec2 right = root - across * waist;

    const HairColors colors = HairPalette(particle.tint);
    const Vec2 rimLeft = root - along * 0.9f + across * (waist + 0.9f);
    const Vec2 rimRight = root - along * 0.9f - across * (waist + 0.9f);
    const Vec2 rimTip = tip + along * 0.9f;
    DrawTriangleAnyWinding(rimLeft, rimRight, rimTip, Fade(colors.edge, 0.85f * fade));
    DrawTriangleAnyWinding(left, right, tip, Fade(colors.fill, fade));
    DrawTriangleAnyWinding(root, left, fork, Fade(colors.fill, 0.9f * fade));
    DrawLineEx(ToRaylib(root), ToRaylib(Lerp(root, tip, 0.7f)), 1.2f, Fade(colors.sheen, 0.55f * fade));
}

void DrawShard(const Particle& particle, bool settled, const Art* art) {
    const float fade = DebrisFade(particle) * (settled ? 0.78f : 1.0f);
    const Vec2 center = VisualPosition(particle);
    const Vec2 along{std::cos(particle.angle), std::sin(particle.angle)};
    const Vec2 across{-along.y, along.x};
    const float skew = 0.35f + 0.4f * Hash(particle.seconds * 19.0f);
    const Vec2 nose = center + along * (particle.size * 0.75f);
    const Vec2 wingA = center - along * (particle.size * 0.45f) + across * (particle.size * skew);
    const Vec2 wingB = center - along * (particle.size * 0.25f) - across * (particle.size * 0.38f);

    const Color base = ShardColor(particle.tint);
    if (!settled && art != nullptr) {
        DrawGlow(*art, center, particle.size * 1.8f, Fade(base, 0.26f * fade));
    }
    DrawTriangleAnyWinding(nose, wingA, wingB, Fade(base, 0.92f * fade));
    DrawLineEx(ToRaylib(nose), ToRaylib(wingA), 1.4f, Fade(Mix(base, kHitFlash, 0.65f), 0.95f * fade));
}

void DrawSpark(const Particle& particle, const Art& art) {
    const float life = LifeLeft(particle);
    const Vec2 head = VisualPosition(particle);
    const Vec2 dir = LengthSquared(particle.velocity) > 1.0f
                         ? Normalized(particle.velocity)
                         : Vec2{std::cos(particle.angle), std::sin(particle.angle)};
    const float length = particle.size * (0.65f + 0.95f * life);
    const Vec2 tail = head - dir * length;
    const Color color = SparkColor(particle.tint);

    DrawGlow(art, head, particle.size * 1.5f, Fade(color, 0.24f * life));
    DrawLineEx(ToRaylib(tail), ToRaylib(head), 2.5f, Fade(color, 0.95f * life));
    DrawCircleV(ToRaylib(head), 1.6f, Fade(Mix(color, kHitFlash, 0.7f), life));
}

void DrawSmoke(const Particle& particle, const Art& art) {
    const float life = LifeLeft(particle);
    const float alpha = 0.34f * life * life;
    const Vec2 center = VisualPosition(particle);
    DrawGlow(art, center, particle.size * 1.6f, Fade(kGunSmoke, alpha));
    DrawCircleV(ToRaylib(center), particle.size * 0.7f, Fade(kGunSmoke, alpha * 0.85f));
}

void DrawDust(const Particle& particle, const Art& art) {
    const float life = LifeLeft(particle);
    const Vec2 center = VisualPosition(particle);
    DrawGlow(art, center, particle.size * 1.65f, Fade(kDust, 0.40f * life));
    DrawCircleV(ToRaylib(center), particle.size * 0.74f, Fade(Mix(kDust, kStone, 0.25f), 0.48f * life));
}

void DrawAirborneShadow(const Particle& particle) {
    if (particle.gravity <= 0.0f || particle.height <= 1.0f) {
        return;
    }
    const float reach = std::clamp(1.0f - particle.height / 70.0f, 0.25f, 1.0f);
    const float radius = std::max(particle.size * 0.42f * reach, 1.4f);
    DrawCircleV(ToRaylib(particle.position), radius, Fade(kDarkness, 0.36f * reach * DebrisFade(particle)));
}

} // namespace

void DrawFloorParticles(const ParticlePool& particles) {
    for (const Particle& particle : particles) {
        if (!IsOnFloor(particle)) {
            continue;
        }
        switch (particle.kind) {
        case ParticleKind::Casing:
            DrawCasing(particle, true);
            break;
        case ParticleKind::Blood:
            DrawBloodSplat(particle);
            break;
        case ParticleKind::Hair:
            DrawHairLock(particle, true);
            break;
        case ParticleKind::Shard:
            DrawShard(particle, true, nullptr);
            break;
        case ParticleKind::Spark:
        case ParticleKind::Smoke:
        case ParticleKind::Dust:
            break;
        }
    }
}

void DrawUnderbodyParticles(const ParticlePool& particles, const Art& art) {
    for (const Particle& particle : particles) {
        if (!IsOnFloor(particle)) {
            DrawAirborneShadow(particle);
        }
        if (particle.kind == ParticleKind::Dust) {
            DrawDust(particle, art);
        }
    }
}

void DrawOverbodyParticles(const ParticlePool& particles, const Art& art) {
    for (const Particle& particle : particles) {
        if (IsOnFloor(particle)) {
            continue;
        }
        switch (particle.kind) {
        case ParticleKind::Spark:
            DrawSpark(particle, art);
            break;
        case ParticleKind::Smoke:
            DrawSmoke(particle, art);
            break;
        case ParticleKind::Casing:
            DrawCasing(particle, false);
            break;
        case ParticleKind::Blood:
            DrawBloodDrop(particle);
            break;
        case ParticleKind::Hair:
            DrawHairLock(particle, false);
            break;
        case ParticleKind::Shard:
            DrawShard(particle, false, &art);
            break;
        case ParticleKind::Dust:
            break;
        }
    }
}

} // namespace horde
