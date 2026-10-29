#include "core/MusicMixer.h"

#include <algorithm>
#include <vector>

namespace horde {

namespace {

// Moves `value` toward `target` by at most `step`.
float Approach(float value, float target, float step) {
    if (value < target) {
        return std::min(value + step, target);
    }
    return std::max(value - step, target);
}

// How far a gain moves in one frame to cross the whole range in `seconds`.
float StepPerFrame(float seconds, int sampleRate) {
    if (seconds <= 0.0f || sampleRate <= 0) {
        return 1.0f;
    }
    return 1.0f / (seconds * static_cast<float>(sampleRate));
}

} // namespace

void CueMusic(MusicMixer& mixer, const MusicCue& cue) {
    mixer.cue = cue;
    if (cue.restart) {
        mixer.position = 0;
        mixer.gains = cue.stems;
    }
}

void MixMusic(MusicMixer& mixer, const MusicTrack& track, std::span<float> out) {
    if (track.loopFrames == 0) {
        std::fill(out.begin(), out.end(), 0.0f);
        return;
    }
    const float stemIn = StepPerFrame(mixer.stats.stemInSeconds, track.sampleRate);
    const float stemOut = StepPerFrame(mixer.stats.stemOutSeconds, track.sampleRate);
    const float volumeStep = StepPerFrame(mixer.cue.volumeSeconds, track.sampleRate);
    for (float& sample : out) {
        mixer.volume = Approach(mixer.volume, mixer.cue.volume, volumeStep);
        float sum = 0.0f;
        for (std::size_t stem = 0; stem < kMusicStemCount; ++stem) {
            float& gain = mixer.gains[stem];
            const float target = mixer.cue.stems[stem];
            gain = Approach(gain, target, target > gain ? stemIn : stemOut);
            const std::vector<float>& samples = track.stems[stem];
            if (gain > 0.0f && mixer.position < samples.size()) {
                sum += gain * samples[mixer.position];
            }
        }
        sample = std::clamp(sum * mixer.volume, -1.0f, 1.0f);
        mixer.position = mixer.position + 1 == track.loopFrames ? 0 : mixer.position + 1;
    }
}

} // namespace horde
