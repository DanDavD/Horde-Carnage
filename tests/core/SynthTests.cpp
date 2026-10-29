#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <doctest/doctest.h>

#include "core/Synth.h"

using horde::SoundRecipe;
using horde::SynthLayer;
using horde::Waveform;

namespace {

constexpr int kRate = horde::kSynthSampleRate;

SoundRecipe OneLayer(SynthLayer layer, float seconds) {
    SoundRecipe recipe{.seconds = seconds};
    recipe.layers[0] = layer;
    recipe.layerCount = 1;
    return recipe;
}

float Peak(const std::vector<float>& samples, std::size_t from = 0, std::size_t to = 0) {
    to = to == 0 ? samples.size() : to;
    float peak = 0.0f;
    for (std::size_t i = from; i < to; ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    return peak;
}

// How many times the wave crosses zero going up: once per cycle for a clean tone.
int UpwardCrossings(const std::vector<float>& samples, std::size_t from, std::size_t to) {
    int crossings = 0;
    for (std::size_t i = from + 1; i < to; ++i) {
        if (samples[i - 1] < 0.0f && samples[i] >= 0.0f) {
            ++crossings;
        }
    }
    return crossings;
}

} // namespace

TEST_CASE("a sound lasts its length, peaks just under full scale and ends in silence") {
    const std::vector<float> samples =
        horde::Synthesize(OneLayer({.waveform = Waveform::Sine, .decay = 10.0f}, 0.5f));
    CHECK(samples.size() == static_cast<std::size_t>(kRate / 2));
    CHECK(Peak(samples) == doctest::Approx(0.89f).epsilon(0.01));
    CHECK(samples.back() == doctest::Approx(0.0f));
}

TEST_CASE("a high-pass thins out a low tone and lets a high one through") {
    // A 60 Hz tone under a 3000 Hz one. Averaging ten samples, one period of the high tone at 30 kHz, leaves
    // only the low one, whose height is then measured.
    const auto lowTone = [](float highPassHz) {
        SoundRecipe recipe{.seconds = 0.5f};
        recipe.layers[0] = {.waveform = Waveform::Sine, .fromHz = 60.0f, .toHz = 60.0f, .decay = 10.0f,
                            .highPassHz = highPassHz};
        recipe.layers[1] = {.waveform = Waveform::Sine, .fromHz = 3000.0f, .toHz = 3000.0f, .decay = 10.0f};
        recipe.layerCount = 2;
        const std::vector<float> samples = horde::Synthesize(recipe, 30000);
        float peak = 0.0f;
        for (std::size_t i = 7500; i + 10 < samples.size(); ++i) {
            float sum = 0.0f;
            for (std::size_t j = i; j < i + 10; ++j) {
                sum += samples[j];
            }
            peak = std::max(peak, std::abs(sum / 10.0f));
        }
        return peak;
    };
    CHECK(lowTone(1200.0f) < 0.25f * lowTone(0.0f));
}

TEST_CASE("a sine plays at its pitch, and a slide ends at its last pitch") {
    const SynthLayer tone{.waveform = Waveform::Sine, .fromHz = 440.0f, .toHz = 440.0f, .decay = 10.0f};
    const std::vector<float> steady = horde::Synthesize(OneLayer(tone, 1.0f));
    CHECK(UpwardCrossings(steady, 0, steady.size()) == doctest::Approx(440).epsilon(0.01));

    // Over the last tenth of a second the slide has settled on 220.
    const std::vector<float> slide = horde::Synthesize(OneLayer(
        {.waveform = Waveform::Sine, .fromHz = 880.0f, .toHz = 220.0f, .decay = 10.0f, .sweepSeconds = 0.5f}, 1.0f));
    const std::size_t tenth = slide.size() / 10;
    CHECK(UpwardCrossings(slide, slide.size() - tenth, slide.size()) == doctest::Approx(22).epsilon(0.1));
}

TEST_CASE("the envelope waits for its delay, then dies away") {
    const std::vector<float> samples = horde::Synthesize(
        OneLayer({.waveform = Waveform::Sine, .delay = 0.1f, .attack = 0.01f, .decay = 0.05f}, 0.6f));
    const std::size_t delay = static_cast<std::size_t>(0.1f * kRate);
    CHECK(Peak(samples, 0, delay - 10) == 0.0f);
    CHECK(Peak(samples, delay, delay + kRate / 20) > 0.5f);
    // Seven time constants after the attack, under a thousandth of the peak.
    const std::size_t late = static_cast<std::size_t>(0.47f * kRate);
    CHECK(Peak(samples, late, samples.size()) < 0.001f);
}

TEST_CASE("a low-pass filter takes the hiss out of noise") {
    const SynthLayer open{.waveform = Waveform::Noise, .decay = 10.0f};
    SynthLayer muffled = open;
    muffled.lowPassFromHz = 200.0f;
    muffled.lowPassToHz = 200.0f;
    // Before scaling to the peak, the change from one sample to the next is how bright noise is. Compared
    // after both are scaled to the same peak, the filtered one moves far less from sample to sample.
    const auto roughness = [](const std::vector<float>& samples) {
        float sum = 0.0f;
        for (std::size_t i = 1; i < samples.size(); ++i) {
            sum += std::abs(samples[i] - samples[i - 1]);
        }
        return sum / static_cast<float>(samples.size());
    };
    CHECK(roughness(horde::Synthesize(OneLayer(muffled, 0.3f))) <
          roughness(horde::Synthesize(OneLayer(open, 0.3f))) * 0.2f);
}

TEST_CASE("the same recipe always makes the same sound, and another seed other noise") {
    SoundRecipe recipe = OneLayer({.waveform = Waveform::Noise, .decay = 0.2f}, 0.2f);
    const std::vector<float> first = horde::Synthesize(recipe);
    CHECK(first == horde::Synthesize(recipe));
    recipe.seed = 99;
    CHECK(first != horde::Synthesize(recipe));
}

TEST_CASE("a recipe with no length or no layers is silence, without failing") {
    CHECK(horde::Synthesize(OneLayer({}, 0.0f)).empty());
    const std::vector<float> empty = horde::Synthesize(SoundRecipe{.seconds = 0.1f});
    CHECK(empty.size() == static_cast<std::size_t>(kRate / 10));
    CHECK(Peak(empty) == 0.0f);
}

TEST_CASE("drive clips the loud part of the wave and the sound still fits in full scale") {
    SoundRecipe recipe = OneLayer({.waveform = Waveform::Sine, .fromHz = 100.0f, .toHz = 100.0f, .decay = 10.0f}, 0.2f);
    recipe.drive = 4.0f;
    const std::vector<float> samples = horde::Synthesize(recipe);
    CHECK(Peak(samples) <= 0.9f);
    // A clipped sine spends more of its time near the peak than a clean one.
    const auto nearPeak = std::count_if(samples.begin(), samples.end(), [](float s) { return std::abs(s) > 0.8f; });
    CHECK(static_cast<float>(nearPeak) / static_cast<float>(samples.size()) > 0.6f);
}
