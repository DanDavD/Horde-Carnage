#pragma once

#include <raylib.h>

#include "app/render/Art.h"
#include "core/Enemy.h"
#include "core/Vec2.h"

namespace horde {

/// How far the longest lock of hair can reach past a hairy body, as a share of its radius. The boss's
/// shield and the Hair Ring's warning are drawn just outside it, so the hair never covers them.
inline constexpr float kTuftLength = 0.5f;

/// How a hairy body looks. The hairballs and the boss are the same drawing in different styles.
struct HairStyle {
    Color fill;  ///< The hair.
    Color edge;  ///< The dark outline, and the strands drawn into the hair.
    Color sheen; ///< The hair on the side the torch lights.
    Color eye;
    int locks;        ///< Locks of hair around the body.
    float lockLength; ///< How far the longest lock reaches past the body, as a share of the radius.
    float bristle;    ///< From 0, soft and matted locks, to 1, stiff and pointed ones.
    /// How far a hit turns the body toward white: all the way for a hairball, which dies in a few hits, but
    /// only part of the way for the boss, which the SMG hits ten times a second and which would otherwise
    /// stay white for the whole fight.
    float flash;
    float eyeSize; ///< Eyes are a share of the radius times this, so a small body's eyes still read.
};

/// How a hairy body stands in this frame.
struct HairPose {
    Vec2 lookAt;                   ///< Where the eyes and the mouth face, usually the player.
    Vec2 light;                    ///< Where the light comes from: the player's torch.
    float seconds = 0.0f;          ///< The fight's clock, for the breathing. A frozen fight stops it.
    Vec2 stretchAlong{1.0f, 0.0f}; ///< A unit vector.
    float stretch = 1.0f;          ///< Above 1, longer along `stretchAlong` and thinner across it; below 1, squashed.
    float mouth = -1.0f;           ///< Below 0 there is no mouth; from 0, a shut snarl, to 1, a roar.
    bool flashing = false;         ///< Just hit.
    bool dead = false;             ///< Still and flat, with its eyes shut.
};

/// `from` moved toward `to` by `amount`, from 0 to 1, channel by channel.
[[nodiscard]] Color Mix(Color from, Color to, float amount);

/// A body of matted hair: uneven locks that breathe, an inner layer of shorter ones, strands drawn into
/// it, a sheen on the side the light comes from, eyes that glow and look at `pose.lookAt`, and a mouth when
/// the pose has one.
void DrawHairBody(Vec2 position, float radius, const HairStyle& style, const HairPose& pose, const Art& art);

/// A hairball, interpolated by `alpha`, swelling and bristling with red eyes for `spitWarningSeconds`
/// before it spits.
void DrawHairball(const Enemy& enemy, Vec2 playerPosition, float spitWarningSeconds, float seconds, float alpha,
                  const Art& art);

} // namespace horde
