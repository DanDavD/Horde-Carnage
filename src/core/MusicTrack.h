#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace horde {

/// The layers of the boss theme. They are one piece, written over the same bars, and always play together
/// from the same point: the music changes by how loud each layer is, never by switching songs.
enum class MusicStem {
    Pulse,       ///< Drums, the guitars and the bass. The whole fight.
    Hymn,        ///< An organ-like choir pad and timpani over the same chords. The second phase.
    Wrath,       ///< Double kick, a lead melody and crashes. The boss close to death, and her frenzy.
    Desperation, ///< A racing heartbeat and strings trembling a half step apart. The player close to death.
    Hook,        ///< A guitar melody over the first phrase. It gives way when the Wrath's lead comes in.
};

inline constexpr std::size_t kMusicStemCount = 5;

/// The short pieces that end a fight, played once over the theme as it stops.
enum class MusicStinger { None, Victory, Defeat };

/// A piece of music ready to play, mono, from -1 to 1, at `sampleRate`: its layers, each exactly `loopFrames`
/// long, and the two stingers.
struct MusicTrack {
    int sampleRate = 0;
    std::size_t loopFrames = 0;
    std::array<std::vector<float>, kMusicStemCount> stems;
    std::vector<float> victory;
    std::vector<float> defeat;
};

} // namespace horde
