#include <array>
#include <cstddef>
#include <vector>

#include <doctest/doctest.h>

#include "core/MusicMixer.h"

using horde::MusicCue;
using horde::MusicMixer;
using horde::MusicStem;
using horde::MusicTrack;

namespace {

// Every frame of every layer holds `values[layer]`.
MusicTrack Steady(int sampleRate, std::size_t frames, std::array<float, horde::kMusicStemCount> values) {
    MusicTrack track{.sampleRate = sampleRate, .loopFrames = frames, .stems = {}};
    for (std::size_t stem = 0; stem < horde::kMusicStemCount; ++stem) {
        track.stems[stem].assign(frames, values[stem]);
    }
    return track;
}

// Every frame holds its own index, a hundredth at a time, the same in every layer.
MusicTrack Ramp(std::size_t frames) {
    MusicTrack track{.sampleRate = 100, .loopFrames = frames, .stems = {}};
    for (std::vector<float>& stem : track.stems) {
        for (std::size_t i = 0; i < frames; ++i) {
            stem.push_back(0.01f * static_cast<float>(i));
        }
    }
    return track;
}

MusicCue Cue(std::array<float, horde::kMusicStemCount> stems, bool restart, float volumeSeconds = 0.0f) {
    return MusicCue{.stems = stems, .volume = 1.0f, .volumeSeconds = volumeSeconds, .restart = restart};
}

} // namespace

TEST_CASE("the mixer is silent until it is cued") {
    MusicMixer mixer;
    const MusicTrack track = Steady(100, 10, {0.5f, 0.5f, 0.5f, 0.5f});
    std::vector<float> out(20, 1.0f);
    horde::MixMusic(mixer, track, out);
    for (const float sample : out) {
        CHECK(sample == doctest::Approx(0.0f));
    }
}

TEST_CASE("a restart plays the cued layers from the first frame of the loop at once") {
    MusicMixer mixer;
    const MusicTrack track = Ramp(10);
    horde::CueMusic(mixer, Cue({1.0f, 0.0f, 0.0f, 0.0f}, true));
    std::vector<float> out(3);
    horde::MixMusic(mixer, track, out);
    CHECK(out[0] == doctest::Approx(0.0f));
    CHECK(out[1] == doctest::Approx(0.01f));
    CHECK(out[2] == doctest::Approx(0.02f));
}

TEST_CASE("the layers play together from one position and wrap round the loop") {
    MusicMixer mixer;
    const MusicTrack track = Ramp(10);
    horde::CueMusic(mixer, Cue({1.0f, 1.0f, 1.0f, 1.0f}, true));
    std::vector<float> out(13);
    horde::MixMusic(mixer, track, out);
    CHECK(out[9] == doctest::Approx(0.36f));
    CHECK(out[10] == doctest::Approx(0.0f));
    CHECK(out[12] == doctest::Approx(0.08f));
}

TEST_CASE("a restart in the middle of the loop goes back to its first frame") {
    MusicMixer mixer;
    const MusicTrack track = Ramp(10);
    horde::CueMusic(mixer, Cue({1.0f, 0.0f, 0.0f, 0.0f}, true));
    std::vector<float> out(5);
    horde::MixMusic(mixer, track, out);
    horde::CueMusic(mixer, Cue({1.0f, 0.0f, 0.0f, 0.0f}, true));
    std::vector<float> next(1);
    horde::MixMusic(mixer, track, next);
    CHECK(next[0] == doctest::Approx(0.0f));
}

TEST_CASE("a layer fades in over its in time, and out over its slower out time") {
    MusicMixer mixer;
    const MusicTrack track = Steady(1000, 4000, {0.0f, 0.5f, 0.0f, 0.0f});
    horde::CueMusic(mixer, Cue({0.0f, 0.0f, 0.0f, 0.0f}, true));
    horde::CueMusic(mixer, Cue({0.0f, 1.0f, 0.0f, 0.0f}, false));
    std::vector<float> rise(300);
    horde::MixMusic(mixer, track, rise);
    // 0.6 s to come all the way in: half way after 0.3 s.
    CHECK(mixer.gains[static_cast<std::size_t>(MusicStem::Hymn)] == doctest::Approx(0.5f).epsilon(0.01));
    CHECK(rise.back() == doctest::Approx(0.25f).epsilon(0.01));
    std::vector<float> full(400);
    horde::MixMusic(mixer, track, full);
    CHECK(mixer.gains[static_cast<std::size_t>(MusicStem::Hymn)] == doctest::Approx(1.0f));

    horde::CueMusic(mixer, Cue({0.0f, 0.0f, 0.0f, 0.0f}, false));
    std::vector<float> fall(750);
    horde::MixMusic(mixer, track, fall);
    // 1.5 s to go all the way out: half way after 0.75 s.
    CHECK(mixer.gains[static_cast<std::size_t>(MusicStem::Hymn)] == doctest::Approx(0.5f).epsilon(0.01));
}

TEST_CASE("the volume moves at the speed the cue asks for") {
    MusicMixer mixer;
    const MusicTrack track = Steady(100, 1000, {0.5f, 0.0f, 0.0f, 0.0f});
    horde::CueMusic(mixer, Cue({1.0f, 0.0f, 0.0f, 0.0f}, true, 2.0f));
    std::vector<float> out(100);
    horde::MixMusic(mixer, track, out);
    CHECK(mixer.volume == doctest::Approx(0.5f));
    CHECK(out.back() == doctest::Approx(0.25f));
}

TEST_CASE("the mix never leaves -1 to 1") {
    MusicMixer mixer;
    const MusicTrack track = Steady(100, 10, {0.9f, 0.9f, 0.9f, -0.9f});
    horde::CueMusic(mixer, Cue({1.0f, 1.0f, 1.0f, 0.0f}, true));
    std::vector<float> out(5);
    horde::MixMusic(mixer, track, out);
    CHECK(out[0] == doctest::Approx(1.0f));
}

TEST_CASE("an empty track plays silence") {
    MusicMixer mixer;
    const MusicTrack track;
    horde::CueMusic(mixer, Cue({1.0f, 1.0f, 1.0f, 1.0f}, true));
    std::vector<float> out(4, 1.0f);
    horde::MixMusic(mixer, track, out);
    CHECK(out[0] == doctest::Approx(0.0f));
}
