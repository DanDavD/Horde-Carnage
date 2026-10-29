#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace horde {

/// The shape of a layer's sound.
enum class Waveform {
    Sine,     ///< Pure and round: thumps, booms, chimes.
    Square,   ///< Hollow and buzzy: clicks, crunch.
    Saw,      ///< Bright and harsh: growls and roars.
    Triangle, ///< Between a sine and a square.
    Noise,    ///< White noise: gunshots, hiss, impacts, whooshes, shaped by the filter.
};

/// One voice of a sound: a waveform whose pitch slides from `fromHz` to `toHz`, under an envelope that waits
/// `delay`, rises over `attack` and then dies away, losing about two thirds every `decay` seconds.
struct SynthLayer {
    Waveform waveform = Waveform::Sine;
    float fromHz = 440.0f;
    float toHz = 440.0f;
    float gain = 1.0f;
    float delay = 0.0f;
    float attack = 0.002f;
    float decay = 0.1f;
    /// How long the pitch takes to slide, and the filter to sweep. 0 slides over the whole sound.
    float sweepSeconds = 0.0f;
    float vibratoHz = 0.0f;    ///< How fast the pitch wavers.
    float vibratoDepth = 0.0f; ///< By how much, as a share of the pitch.
    /// A low-pass filter on the layer that sweeps from `lowPassFromHz` to `lowPassToHz`. 0 leaves it open.
    float lowPassFromHz = 0.0f;
    float lowPassToHz = 0.0f;
    /// A high-pass filter that thins out what is under it, before the mix is clipped: a distorted guitar stays
    /// tight instead of turning to mud. 0 leaves it off.
    float highPassHz = 0.0f;
};

/// The most layers a sound can stack.
inline constexpr int kMaxSynthLayers = 4;

/// A whole sound, made in code instead of recorded: up to four layers mixed, pushed through a soft clip for
/// grit, and scaled to a peak just under full scale.
struct SoundRecipe {
    std::array<SynthLayer, kMaxSynthLayers> layers{};
    int layerCount = 0;
    float seconds = 0.5f;
    float drive = 1.0f; ///< Above 1, the mix is clipped softly: louder and dirtier.
    std::uint64_t seed = 1; ///< For the noise, so a sound is the same every time the game starts.
};

/// The sample rate of every sound the game makes.
inline constexpr int kSynthSampleRate = 44100;

/// The samples of `recipe`, mono, from -1 to 1, at `sampleRate`. It allocates, so the game calls it once per
/// sound while loading, never during the fight. The same recipe always gives the same samples.
[[nodiscard]] std::vector<float> Synthesize(const SoundRecipe& recipe, int sampleRate = kSynthSampleRate);

} // namespace horde
