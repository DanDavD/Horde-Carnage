#include "app/audio/MusicPlayer.h"

#include <span>

#include "core/BossTheme.h"

namespace horde {

namespace {

// The frames raylib asks for at a time, about a tenth of a second. A frame of the game that takes longer than
// one block (a hitch while loading) still has the other block queued, so the music does not stutter.
constexpr int kBlockFrames = 4096;
// The music sits under the effects: a warning or a hit must always cut through it.
constexpr float kMusicVolume = 0.7f;

} // namespace

MusicPlayer LoadMusicPlayer() {
    MusicPlayer player;
    if (!IsAudioDeviceReady()) {
        return player;
    }
    player.track = ComposeBossTheme();
    player.block.assign(kBlockFrames, 0.0f);
    SetAudioStreamBufferSizeDefault(kBlockFrames);
    player.stream = LoadAudioStream(static_cast<unsigned int>(player.track.sampleRate), 32, 1);
    SetAudioStreamBufferSizeDefault(0);
    if (!IsAudioStreamValid(player.stream)) {
        return player;
    }
    SetAudioStreamVolume(player.stream, kMusicVolume);
    PlayAudioStream(player.stream);
    player.ready = true;
    return player;
}

void UnloadMusicPlayer(MusicPlayer& player) {
    if (!player.ready) {
        return;
    }
    StopAudioStream(player.stream);
    UnloadAudioStream(player.stream);
    player.ready = false;
}

void UpdateMusicPlayer(MusicPlayer& player, const Session& session, float frameSeconds) {
    if (!player.ready) {
        return;
    }
    CueMusic(player.mixer, StepMusicDirector(player.director, SituationOf(session), frameSeconds));
    // Both halves of the stream's buffer can be empty at once, after a long frame: fill both.
    while (IsAudioStreamProcessed(player.stream)) {
        MixMusic(player.mixer, player.track, std::span<float>(player.block));
        UpdateAudioStream(player.stream, player.block.data(), kBlockFrames);
    }
}

} // namespace horde
