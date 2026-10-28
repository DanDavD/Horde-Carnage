#pragma once

#include "core/Vec2.h"

namespace horde {

/// Something that happened in the fight that the player should feel, see or hear: a hit, a break, a landing.
enum class WorldEventKind {
    ShotFired,       ///< The player fired. At the muzzle, along the shot. `amount` is the gun's damage.
    DashStarted,     ///< The player started a dash. Where it left from, along the dash.
    PlayerHurt,      ///< A hit landed on the player and left them alive. `amount` is the damage.
    PlayerKilled,    ///< The hit that emptied the player's bar.
    BossHurt,        ///< A bullet took health from the exposed boss. Where it hit, along the bullet.
    ShieldStruck,    ///< A bullet met the shield without breaking it. `amount` is 0 when it bounced off.
    ShieldBroken,    ///< The bullet that broke the shield.
    HairballKilled,  ///< A hairball died. Where it was, along the bullet that killed it.
    BossLanded,      ///< The boss came down from a Leap.
    BossHitWall,     ///< A Charge ended against a wall, stunning the boss. Along the run.
    FirstPhaseEnded, ///< The first phase's bar ran out, and the transformation began.
    BossRoared,      ///< The roar that ends the transformation, or the one that starts the frenzy.
    BossDefeated,    ///< The second phase's bar ran out: the fight is won.
};

/// One thing that happened, where it happened and, when it has one, which way it went.
///
/// The simulation writes these and never reads them. The game reads them after each step and turns them
/// into a hit stop, a screen shake and particles, and later into sounds, so none of that has to live in
/// the rules.
struct WorldEvent {
    WorldEventKind kind = WorldEventKind::ShotFired;
    Vec2 position;
    Vec2 direction; ///< A unit vector, or zero when the event has no direction.
    int amount = 0; ///< The damage, for the events that deal or take it.
};

} // namespace horde
