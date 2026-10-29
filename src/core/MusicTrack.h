#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace horde {

/// The layers of the boss theme. They are one piece, written over the same bars, and always play together
/// from the same point: the music changes by how loud each layer is, never by switching songs.
enum class MusicStem {
    Pulse,       ///< Drums, the galloping guitar and the bass. The whole fight.
    Hymn,        ///< An organ-like choir pad and timpani over the same chords. The second phase.
    Wrath,       ///< Double kick, a lead melody and crashes. The boss close to death, and her frenzy.
    Desperation, ///< A racing heartbeat and strings trembling a half step apart. The player close to death.
};

inline constexpr std::size_t kMusicStemCount = 4;

/// A piece of music ready to play: its layers, each exactly `loopFrames` long, mono, from -1 to 1, at
/// `sampleRate`.
struct MusicTrack {
    int sampleRate = 0;
    std::size_t loopFrames = 0;
    std::array<std::vector<float>, kMusicStemCount> stems;
};

} // namespace horde
