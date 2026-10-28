#pragma once

#include <cstddef>
#include <optional>

#include "core/Random.h"
#include "core/Vec2.h"
#include "core/WorldEvent.h"

namespace horde {

/// Every sound the fight makes. Each one is synthesized once when the game starts (see `SoundLibrary`).
enum class SoundId {
    PistolShot,
    SmgShot,
    ShellPing,     ///< A bullet bouncing off the blue Shell.
    ShieldKnock,   ///< A bullet the shield soaks up without breaking.
    ShieldShatter, ///< The shield breaking.
    BossFlesh,     ///< A bullet in the exposed boss.
    HairballBurst, ///< A hairball dying.
    Dash,
    FlaskDrink,
    PlayerHurt,
    PlayerHeavyHurt, ///< The Charge and the Leap's impact.
    PlayerDeath,
    BossLanding,
    WallCrash,
    BossRoar,
    BossCollapse,  ///< The first bar running out, as the transformation starts.
    BossDeath,
    ChargeGrowl,   ///< The warning of a Charge.
    RingSwell,     ///< The warning of a Hair Ring.
    LeapGrunt,     ///< The warning of a Leap.
    VolleyHiss,    ///< The warning of a Volley.
    SummonHowl,    ///< The warning of a Summon.
    Spit,          ///< Strands leaving the boss or a hairball.
    Count,
};

inline constexpr std::size_t kSoundCount = static_cast<std::size_t>(SoundId::Count);

/// One sound to play, and how: its volume from 0 to 1, its pitch (1 for as made), and where it sits between
/// the speakers, from -1 all left to 1 all right.
struct SoundCue {
    SoundId id = SoundId::PistolShot;
    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
};

/// How far a sound can lean to one side: never all the way, so nothing plays in one ear only.
inline constexpr float kMaxPan = 0.7f;

/// The sound an event makes, if it makes one. Every cue gets a little random pitch and volume from `random`,
/// so the SMG's tenth shot in a second does not sound like a copy of the first. `random` is the sound's own
/// generator, never the fight's.
[[nodiscard]] std::optional<SoundCue> CueFor(const WorldEvent& event, Random& random);

/// `cue` placed for a listener at `listener` (the middle of the screen) who sees `viewWidth` across: leaning
/// toward the side the sound comes from, all the way to `kMaxPan` at the screen's edge, and quieter the
/// further off screen it is, down to half beyond a view's width.
[[nodiscard]] SoundCue Placed(SoundCue cue, Vec2 source, Vec2 listener, float viewWidth);

} // namespace horde
