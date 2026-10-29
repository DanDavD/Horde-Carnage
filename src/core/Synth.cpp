#include "core/Synth.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "core/Random.h"

namespace horde {

namespace {

constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
// The loudest sample after scaling: a little room under full scale, so mixed sounds clip less.
constexpr float kPeak = 0.89f;
// A short fade at the very end, so a sound cut off mid-wave does not click.
constexpr float kFadeOutSeconds = 0.004f;

// An exponential slide from `from` to `to`, the way pitch and brightness are heard: an octave is an octave.
float Slide(float from, float to, float progress) {
    if (from <= 0.0f || to <= 0.0f) {
        return from + (to - from) * progress;
    }
    return from * std::pow(to / from, progress);
}

float Oscillate(Waveform waveform, float phase, Random& noise) {
    switch (waveform) {
    case Waveform::Sine:
        return std::sin(kTwoPi * phase);
    case Waveform::Square:
        return phase < 0.5f ? 1.0f : -1.0f;
    case Waveform::Saw:
        return 2.0f * phase - 1.0f;
    case Waveform::Triangle:
        return phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
    case Waveform::Noise:
        return RandomRange(noise, -1.0f, 1.0f);
    }
    return 0.0f;
}

float Envelope(const SynthLayer& layer, float t) {
    const float since = t - layer.delay;
    if (since < 0.0f) {
        return 0.0f;
    }
    if (since < layer.attack) {
        return since / layer.attack;
    }
    return layer.decay > 0.0f ? std::exp(-(since - layer.attack) / layer.decay) : 0.0f;
}

// Adds one layer into `mix`.
void Render(const SynthLayer& layer, std::vector<float>& mix, float seconds, int sampleRate, Random& noise) {
    const float dt = 1.0f / static_cast<float>(sampleRate);
    const float sweep = layer.sweepSeconds > 0.0f ? layer.sweepSeconds : seconds;
    float phase = 0.0f;
    float filtered = 0.0f;
    // The high-pass keeps what the input changed by since the last sample, and lets the rest leak away.
    const float keep = layer.highPassHz > 0.0f ? 1.0f / (1.0f + kTwoPi * layer.highPassHz * dt) : 0.0f;
    float lastInput = 0.0f;
    float highPassed = 0.0f;
    for (std::size_t i = 0; i < mix.size(); ++i) {
        const float t = static_cast<float>(i) * dt;
        const float progress = std::min(t / sweep, 1.0f);
        float hz = Slide(layer.fromHz, layer.toHz, progress);
        if (layer.vibratoDepth > 0.0f) {
            hz *= 1.0f + layer.vibratoDepth * std::sin(kTwoPi * layer.vibratoHz * t);
        }
        phase += hz * dt;
        phase -= std::floor(phase);
        float sample = Oscillate(layer.waveform, phase, noise);
        if (layer.lowPassFromHz > 0.0f) {
            // A one-pole low-pass: each sample moves part of the way toward the input, more for a higher
            // cutoff, which takes the hiss off noise and turns it into a thump or a whoosh.
            const float cutoff = Slide(layer.lowPassFromHz, layer.lowPassToHz, progress);
            const float share = 1.0f - std::exp(-kTwoPi * cutoff * dt);
            filtered += share * (sample - filtered);
            sample = filtered;
        }
        if (layer.highPassHz > 0.0f) {
            highPassed = keep * (highPassed + sample - lastInput);
            lastInput = sample;
            sample = highPassed;
        }
        mix[i] += sample * layer.gain * Envelope(layer, t);
    }
}

} // namespace

std::vector<float> Synthesize(const SoundRecipe& recipe, int sampleRate) {
    const auto count = static_cast<std::size_t>(std::max(recipe.seconds, 0.0f) * static_cast<float>(sampleRate));
    std::vector<float> mix(count, 0.0f);
    if (count == 0) {
        return mix;
    }
    Random noise = SeedRandom(recipe.seed, 3);
    for (int i = 0; i < std::min(recipe.layerCount, kMaxSynthLayers); ++i) {
        Render(recipe.layers[static_cast<std::size_t>(i)], mix, recipe.seconds, sampleRate, noise);
    }

    // The soft clip rounds off the loudest part of the wave, like an overdriven speaker, which is what gives
    // a gunshot or a roar its grit. Scaled by tanh(drive), so a drive of 1 barely changes a quiet mix.
    if (recipe.drive > 1.0f) {
        const float scale = 1.0f / std::tanh(recipe.drive);
        for (float& sample : mix) {
            sample = std::tanh(sample * recipe.drive) * scale;
        }
    }
    float loudest = 0.0f;
    for (const float sample : mix) {
        loudest = std::max(loudest, std::abs(sample));
    }
    const float gain = loudest > 0.0f ? kPeak / loudest : 0.0f;
    const auto fade = std::min(count, static_cast<std::size_t>(kFadeOutSeconds * static_cast<float>(sampleRate)));
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t fromEnd = count - 1 - i;
        const float tail = fromEnd < fade ? static_cast<float>(fromEnd) / static_cast<float>(fade) : 1.0f;
        mix[i] *= gain * tail;
    }
    return mix;
}

} // namespace horde
