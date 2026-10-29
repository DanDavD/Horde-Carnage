#pragma once

#include <vector>

#include <raylib.h>

#include "core/MusicDirector.h"
#include "core/MusicMixer.h"
#include "core/MusicTrack.h"
#include "core/Session.h"

namespace horde {

/// The boss theme, composed once, and the raylib stream it plays through. The game mixes the layers itself and
/// hands raylib one finished stream, a block at a time, so the layers share a single position.
struct MusicPlayer {
    MusicTrack track;
    MusicDirector director;
    MusicMixer mixer;
    AudioStream stream{};
    std::vector<float> block; ///< One block of the stream, mixed before it is handed over.
    bool ready = false;       ///< False when there is no audio device; then nothing plays and nothing fails.
};

/// Composes the boss theme and opens its stream, silent until a fight begins. Needs `InitAudioDevice` first.
[[nodiscard]] MusicPlayer LoadMusicPlayer();

/// Frees what `LoadMusicPlayer` opened. Needs the audio device still open.
void UnloadMusicPlayer(MusicPlayer& player);

/// Once a frame: reads `session`, cues the mixer, and refills whatever the stream has played since the last
/// frame.
void UpdateMusicPlayer(MusicPlayer& player, const Session& session, float frameSeconds);

/// Sets the player's music volume, from 0 to 1, under raylib's master volume.
void SetMusicPlayerVolume(MusicPlayer& player, float gain);

} // namespace horde
