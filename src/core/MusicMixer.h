#pragma once

#include <array>
#include <cstddef>
#include <span>

#include "core/MusicDirector.h"
#include "core/MusicTrack.h"

namespace horde {

struct MusicMixerStats {
    float stemInSeconds = 0.6f;  ///< A layer takes this long to come all the way in.
    float stemOutSeconds = 1.5f; ///< And this long to go all the way out: the music winds down slower than it builds.
};

/// Plays the layers of a track together, from one position, so they can never drift apart, and moves each
/// layer's gain toward its cue a sample at a time, so a change is a fade and never a click. A stinger plays
/// once over the loop, with a volume of its own.
struct MusicMixer {
    MusicMixerStats stats;
    std::size_t position = 0; ///< The frame of the loop that plays next.
    std::array<float, kMusicStemCount> gains{};
    float volume = 0.0f;
    MusicStinger stinger = MusicStinger::None; ///< The last stinger started.
    std::size_t stingerPosition = 0;           ///< Its next frame; past its end, it is over.
    float stingerGain = 0.0f;
    MusicCue cue; ///< Where the gains and the volumes are heading.
};

/// Points the mixer at `cue`. A restart goes back to the first frame of the loop, with the layers already at
/// their levels; a stinger starts from its first frame.
void CueMusic(MusicMixer& mixer, const MusicCue& cue);

/// Fills `out` with the next frames of `track`, mono, and moves the positions and the gains on. The loop wraps
/// round seamlessly; a stinger plays once. The output never leaves -1 to 1.
void MixMusic(MusicMixer& mixer, const MusicTrack& track, std::span<float> out);

} // namespace horde
