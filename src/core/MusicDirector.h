#pragma once

#include <array>

#include "core/Boss.h"
#include "core/MusicTrack.h"
#include "core/Session.h"

namespace horde {

/// What the music listens to: a few numbers read from the session, so the director can be tested without a
/// whole fight.
struct MusicSituation {
    SessionState state = SessionState::Title;
    BossPhase bossPhase = BossPhase::One;
    bool bossFrenzied = false;
    float bossHealthFraction = 1.0f; ///< Of the bar in progress: each phase has its own.
    int playerHealth = 100;
};

/// The situation of `session` right now.
[[nodiscard]] MusicSituation SituationOf(const Session& session);

/// What the mixer is asked to play: how loud each layer and the whole theme should be.
struct MusicCue {
    std::array<float, kMusicStemCount> stems{}; ///< From 0 to 1.
    float volume = 0.0f;                        ///< The whole theme, from 0 to 1.
    float volumeSeconds = 1.0f;                 ///< How long the volume takes to move all the way, 0 to 1.
    /// Starts the theme over from its first bar, with the layers already at their levels: a new fight, or the
    /// second phase coming in after the roar.
    bool restart = false;
    /// Starts this stinger from its first frame, at `stingerVolume` at once: the fight has just ended.
    MusicStinger startStinger = MusicStinger::None;
    float stingerVolume = 0.0f;         ///< The stinger playing, if any, from 0 to 1.
    float stingerVolumeSeconds = 0.05f; ///< How long the stinger's volume takes to move all the way.
};

struct MusicDirectorStats {
    /// The share of the boss's second bar where the Wrath starts to come in, and where it is all the way in.
    /// Her frenzy brings it all the way in at once. The first bar running low changes nothing.
    float wrathFromFraction = 0.35f;
    float wrathFullFraction = 0.15f;
    /// The player's health where the Desperation starts to come in, and where it is all the way in.
    int desperationFromHealth = 45;
    int desperationFullHealth = 30;
    /// A layer that has come in stays at least this long before it goes down again, so a flask or a hit right
    /// at the threshold does not make it flicker in and out.
    float holdSeconds = 2.5f;
    float pausedVolume = 0.35f;       ///< Behind the pause menu, the fight goes on quietly.
    float transformingVolume = 0.15f; ///< The band drops out while she roars.
    float fightVolumeSeconds = 0.1f;  ///< Back up fast: the second phase slams in.
    float pauseVolumeSeconds = 0.3f;
    /// The theme gets out of the way of the stinger, which starts on the same frame.
    float deathVolumeSeconds = 0.15f;
    float victoryVolumeSeconds = 0.4f;
    float titleVolumeSeconds = 1.0f;  ///< The theme and a stinger still playing fade out on the title.
    float stingerCutSeconds = 0.05f;  ///< A retry cuts the stinger short, without a click.
};

/// Decides what the boss theme should do, frame after frame. Only the levels it holds and what it last saw
/// are kept between frames.
struct MusicDirector {
    MusicDirectorStats stats;
    std::array<float, kMusicStemCount> levels{};
    std::array<float, kMusicStemCount> holdSecondsLeft{};
    bool inFight = false; ///< Playing or paused at the last call.
    BossPhase bossPhase = BossPhase::One;
};

/// The cue for `situation`, `dt` seconds after the last call:
///
/// - **Pulse** plays through the whole fight. **Hymn** joins in the second phase. **Wrath** comes in as the
///   boss's second bar runs low, and all the way in her frenzy; the first phase never brings it in. **Desperation** comes in as the player's health runs
///   low. A layer comes in at once and goes down only after `holdSeconds`. **Hook** gives way to the Wrath in
///   an equal-power crossfade, so the two melodies never play over each other at full and the loudness holds
///   while one hands over to the other.
/// - A new fight starts the theme over with the levels of the last one forgotten, and the end of the boss's
///   transformation starts it over too, with the hymn in.
/// - The volume drops behind the pause menu and while the boss transforms, and the theme fades out on the
///   title screen. A death starts the Defeat stinger and a victory the Victory stinger, while the theme gets
///   out of their way with its layers as they were; a retry cuts the stinger, and the title fades it.
[[nodiscard]] MusicCue StepMusicDirector(MusicDirector& director, const MusicSituation& situation, float dt);

} // namespace horde
