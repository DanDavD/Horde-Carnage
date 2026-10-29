#include "core/BossTheme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <vector>

#include "core/Sequencer.h"

namespace horde {

namespace {

constexpr int kLoopSteps = kBossThemeBars * kStepsPerBar;
// The loudest the four layers ever get together, like the sound effects' peak.
constexpr float kPeak = 0.89f;

enum class Quality { Minor, Major };

struct Chord {
    int root; ///< MIDI note in the guitar's register, from D2 to C3.
    Quality quality;
};

constexpr int kD = 38;
constexpr int kF = 41;
constexpr int kG = 43;
constexpr int kA = 45;
constexpr int kBb = 46;
constexpr int kC = 48;

// One chord a bar. The first phrase walks i, VI, VII and lands on a major V, the leading tone that makes
// D minor sound like a threat about to resolve. The second phrase moves every bar, so the loop tightens before
// it comes round.
constexpr std::array<Chord, kBossThemeBars> kChords{{
    {kD, Quality::Minor},
    {kD, Quality::Minor},
    {kBb, Quality::Major},
    {kBb, Quality::Major},
    {kC, Quality::Major},
    {kC, Quality::Major},
    {kA, Quality::Major},
    {kA, Quality::Major},
    {kD, Quality::Minor},
    {kC, Quality::Major},
    {kBb, Quality::Major},
    {kA, Quality::Major},
    {kD, Quality::Minor},
    {kF, Quality::Major},
    {kG, Quality::Minor},
    {kA, Quality::Major},
}};

struct MelodyNote {
    int pitch; ///< MIDI note.
    int steps; ///< Length in sixteenths.
};

// The Wrath's lead, in the order it plays. It sits on the chord tones, and the C sharp of each A major bar
// pulls back toward D.
constexpr MelodyNote kMelody[] = {
    // Phrase one.
    {74, 4}, {69, 2}, {74, 2}, {77, 4}, {76, 2}, {74, 2},           // Dm
    {69, 4}, {65, 2}, {69, 2}, {74, 8},                             // Dm
    {77, 4}, {74, 2}, {70, 2}, {77, 4}, {79, 4},                    // Bb
    {77, 6}, {74, 2}, {70, 8},                                      // Bb
    {76, 4}, {72, 2}, {67, 2}, {76, 4}, {77, 4},                    // C
    {79, 6}, {77, 2}, {76, 8},                                      // C
    {73, 4}, {76, 4}, {81, 4}, {79, 2}, {76, 2},                    // A
    {73, 4}, {69, 4}, {76, 4}, {73, 4},                             // A
    // Phrase two: an arpeggio up and back down on every bar.
    {74, 2}, {77, 2}, {81, 4}, {77, 2}, {74, 2}, {69, 4},           // Dm
    {76, 2}, {79, 2}, {84, 4}, {79, 2}, {76, 2}, {72, 4},           // C
    {74, 2}, {77, 2}, {82, 4}, {77, 2}, {74, 2}, {70, 4},           // Bb
    {73, 2}, {76, 2}, {81, 4}, {79, 2}, {76, 2}, {73, 4},           // A
    {74, 4}, {81, 4}, {77, 4}, {74, 4},                             // Dm
    {72, 4}, {77, 4}, {81, 4}, {77, 4},                             // F
    {82, 4}, {79, 4}, {74, 4}, {70, 4},                             // Gm
    {69, 2}, {73, 2}, {76, 2}, {81, 2}, {79, 2}, {76, 2}, {73, 4},  // A, running up into the top of the loop
};

constexpr int MelodySteps() {
    int steps = 0;
    for (const MelodyNote& note : kMelody) {
        steps += note.steps;
    }
    return steps;
}
static_assert(MelodySteps() == kLoopSteps, "the melody must fill the loop exactly");

SoundRecipe Recipe(float seconds, float drive, std::uint64_t seed, std::initializer_list<SynthLayer> layers) {
    SoundRecipe recipe{.seconds = seconds, .drive = drive, .seed = seed};
    for (const SynthLayer& layer : layers) {
        if (recipe.layerCount == kMaxSynthLayers) {
            break;
        }
        recipe.layers[static_cast<std::size_t>(recipe.layerCount++)] = layer;
    }
    return recipe;
}

// --- The instruments. Every one is a sound recipe, like the effects in SoundLibrary. ---

// The click of the beater carries the kick on small speakers, which cannot play its low thump.
SoundRecipe Kick() {
    return Recipe(0.3f, 2.5f, 101,
                  {{.waveform = Waveform::Sine, .fromHz = 180.0f, .toHz = 55.0f, .decay = 0.12f, .sweepSeconds = 0.06f},
                   {.waveform = Waveform::Noise, .gain = 0.9f, .decay = 0.01f, .lowPassFromHz = 8000.0f,
                    .lowPassToHz = 2500.0f},
                   {.waveform = Waveform::Sine, .fromHz = 62.0f, .toHz = 55.0f, .gain = 0.25f, .decay = 0.15f}});
}

// The Wrath's double kick: shorter, tighter and higher, so sixteen a bar stay apart and are heard over the
// gallop.
SoundRecipe DoubleKick() {
    return Recipe(0.1f, 2.0f, 102,
                  {{.waveform = Waveform::Sine, .fromHz = 200.0f, .toHz = 70.0f, .decay = 0.05f, .sweepSeconds = 0.03f},
                   {.waveform = Waveform::Noise, .gain = 1.0f, .decay = 0.006f, .lowPassFromHz = 7000.0f,
                    .lowPassToHz = 2500.0f}});
}

SoundRecipe Snare() {
    return Recipe(0.3f, 1.6f, 103,
                  {{.waveform = Waveform::Noise, .decay = 0.1f, .sweepSeconds = 0.1f, .lowPassFromHz = 9000.0f,
                    .lowPassToHz = 3500.0f},
                   {.waveform = Waveform::Triangle, .fromHz = 230.0f, .toHz = 180.0f, .gain = 0.7f, .decay = 0.05f},
                   {.waveform = Waveform::Noise, .gain = 0.4f, .decay = 0.04f, .lowPassFromHz = 2500.0f,
                    .lowPassToHz = 2500.0f}});
}

SoundRecipe HiHat() {
    return Recipe(0.07f, 1.0f, 104, {{.waveform = Waveform::Noise, .decay = 0.018f}});
}

SoundRecipe Crash() {
    return Recipe(1.6f, 1.0f, 105,
                  {{.waveform = Waveform::Noise, .attack = 0.003f, .decay = 0.45f, .sweepSeconds = 1.2f,
                    .lowPassFromHz = 14000.0f, .lowPassToHz = 5000.0f},
                   {.waveform = Waveform::Noise, .gain = 0.5f, .decay = 0.08f}});
}

// A palm-muted power chord: the root, a detuned double and the fifth on saws, pushed hard into the soft clip,
// which is where the distortion and the growl between the root and the fifth come from. The low end is thinned
// out before the clip, as a metal player's boost pedal does, so the chords stay tight and the crunch comes
// through. The filter closes as it dies, like a muted string. The first chug of a bar rings longer and brighter.
SoundRecipe Chug(int root, bool accent) {
    const float hz = NoteHz(root);
    const float decay = accent ? 0.16f : 0.1f;
    const float open = accent ? 6500.0f : 5000.0f;
    const float closed = accent ? 2500.0f : 1600.0f;
    constexpr float kTight = 180.0f;
    return Recipe(0.19f, 6.0f, 200 + static_cast<std::uint64_t>(root),
                  {{.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz, .decay = decay, .sweepSeconds = 0.12f,
                    .lowPassFromHz = open, .lowPassToHz = closed, .highPassHz = kTight},
                   {.waveform = Waveform::Saw, .fromHz = hz * 1.007f, .toHz = hz * 1.007f, .gain = 0.8f,
                    .decay = decay, .sweepSeconds = 0.12f, .lowPassFromHz = open, .lowPassToHz = closed,
                    .highPassHz = kTight},
                   {.waveform = Waveform::Saw, .fromHz = hz * 1.4983f, .toHz = hz * 1.4983f, .gain = 0.8f,
                    .decay = decay, .sweepSeconds = 0.12f, .lowPassFromHz = open, .lowPassToHz = closed,
                    .highPassHz = kTight},
                   {.waveform = Waveform::Square, .fromHz = hz * 2.0f, .toHz = hz * 2.0f, .gain = 0.35f,
                    .decay = decay, .sweepSeconds = 0.12f, .lowPassFromHz = open, .lowPassToHz = closed,
                    .highPassHz = kTight}});
}

// An octave under the guitar, dark, doubling its eighths.
SoundRecipe Bass(int root) {
    const float hz = NoteHz(root - 12);
    return Recipe(0.19f, 1.8f, 300 + static_cast<std::uint64_t>(root),
                  {{.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz, .decay = 0.14f, .lowPassFromHz = 900.0f,
                    .lowPassToHz = 400.0f},
                   {.waveform = Waveform::Sine, .fromHz = hz, .toHz = hz, .gain = 0.5f, .decay = 0.16f}});
}

// A timpani tuned to the chord: the skin drops a little in pitch as it is struck.
SoundRecipe Timpani(int root) {
    const float hz = NoteHz(root);
    return Recipe(1.4f, 1.3f, 400 + static_cast<std::uint64_t>(root),
                  {{.waveform = Waveform::Sine, .fromHz = hz * 1.08f, .toHz = hz, .decay = 0.45f, .sweepSeconds = 0.08f},
                   {.waveform = Waveform::Sine, .fromHz = hz * 1.5f, .toHz = hz * 1.5f, .gain = 0.3f, .decay = 0.25f},
                   {.waveform = Waveform::Noise, .gain = 0.5f, .decay = 0.03f, .lowPassFromHz = 600.0f,
                    .lowPassToHz = 600.0f}});
}

// One voice of the hymn: triangles a little out of tune with each other, each with its own slow waver, which
// is what makes a pad sound like many voices instead of one. It swells in and lasts the bar.
SoundRecipe HymnVoice(int pitch, float seconds) {
    const float hz = NoteHz(pitch);
    return Recipe(seconds, 1.0f, 500 + static_cast<std::uint64_t>(pitch),
                  {{.waveform = Waveform::Triangle, .fromHz = hz, .toHz = hz, .attack = 0.3f, .decay = 3.0f,
                    .vibratoHz = 5.0f, .vibratoDepth = 0.004f},
                   {.waveform = Waveform::Triangle, .fromHz = hz * 1.006f, .toHz = hz * 1.006f, .gain = 0.8f,
                    .attack = 0.35f, .decay = 3.0f, .vibratoHz = 4.6f, .vibratoDepth = 0.005f},
                   {.waveform = Waveform::Sine, .fromHz = hz * 0.995f, .toHz = hz * 0.995f, .gain = 0.6f,
                    .attack = 0.3f, .decay = 3.0f},
                   {.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz, .gain = 0.5f, .attack = 0.4f, .decay = 3.0f,
                    .lowPassFromHz = 2500.0f, .lowPassToHz = 2500.0f}});
}

// The lead: a saw and a square through the clip, an octave under them for weight, with a singer's vibrato.
SoundRecipe Lead(int pitch, float seconds) {
    const float hz = NoteHz(pitch);
    return Recipe(seconds, 2.2f, 600 + static_cast<std::uint64_t>(pitch),
                  {{.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz, .attack = 0.008f, .decay = 1.5f,
                    .vibratoHz = 5.5f, .vibratoDepth = 0.006f, .lowPassFromHz = 4200.0f, .lowPassToHz = 4200.0f},
                   {.waveform = Waveform::Square, .fromHz = hz * 1.004f, .toHz = hz * 1.004f, .gain = 0.45f,
                    .attack = 0.008f, .decay = 1.5f, .vibratoHz = 5.5f, .vibratoDepth = 0.006f,
                    .lowPassFromHz = 4200.0f, .lowPassToHz = 4200.0f},
                   {.waveform = Waveform::Saw, .fromHz = hz * 0.5f, .toHz = hz * 0.5f, .gain = 0.35f,
                    .attack = 0.008f, .decay = 1.5f, .lowPassFromHz = 2000.0f, .lowPassToHz = 2000.0f}});
}

// A heartbeat: a soft thump, with a dull knock on top so it is still heard on speakers too small for the thump.
SoundRecipe Heartbeat() {
    return Recipe(0.25f, 1.5f, 700,
                  {{.waveform = Waveform::Sine, .fromHz = 110.0f, .toHz = 55.0f, .gain = 0.6f, .decay = 0.09f,
                    .sweepSeconds = 0.05f},
                   {.waveform = Waveform::Triangle, .fromHz = 240.0f, .toHz = 150.0f, .gain = 0.6f, .attack = 0.004f,
                    .decay = 0.03f, .sweepSeconds = 0.03f},
                   {.waveform = Waveform::Noise, .gain = 0.3f, .decay = 0.015f, .lowPassFromHz = 900.0f,
                    .lowPassToHz = 900.0f}});
}

// Strings trembling on two notes a half step apart, the most uneasy interval there is. One short stroke; the
// tremolo is a stroke on every sixteenth.
SoundRecipe Tremolo(int pitch) {
    const float hz = NoteHz(pitch);
    const float above = NoteHz(pitch + 1);
    return Recipe(0.1f, 1.2f, 800 + static_cast<std::uint64_t>(pitch),
                  {{.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz, .attack = 0.004f, .decay = 0.07f,
                    .vibratoHz = 6.0f, .vibratoDepth = 0.005f, .lowPassFromHz = 3200.0f, .lowPassToHz = 3200.0f},
                   {.waveform = Waveform::Saw, .fromHz = above, .toHz = above, .attack = 0.004f, .decay = 0.07f,
                    .vibratoHz = 6.4f, .vibratoDepth = 0.005f, .lowPassFromHz = 3200.0f, .lowPassToHz = 3200.0f}});
}

// --- Voicing. ---

// `pitchClass` (0 to 11) moved into the octave that starts at `low`.
int InRange(int pitchClass, int low) {
    int note = low + ((pitchClass - low) % 12 + 12) % 12;
    return note;
}

int ThirdOf(const Chord& chord) {
    return chord.root + (chord.quality == Quality::Minor ? 3 : 4);
}

int FifthOf(const Chord& chord) {
    return chord.root + 7;
}

// Every note is synthesized once and then copied wherever it plays, which keeps composing quick.
struct NoteCache {
    int sampleRate;
    std::map<std::uint64_t, std::vector<float>> sounds;

    const std::vector<float>& Get(std::uint64_t key, const SoundRecipe& recipe) {
        auto found = sounds.find(key);
        if (found == sounds.end()) {
            found = sounds.emplace(key, Synthesize(recipe, sampleRate)).first;
        }
        return found->second;
    }
};

enum Instrument : std::uint64_t {
    kKick = 1,
    kDoubleKick,
    kSnare,
    kHiHat,
    kCrash,
    kChug,
    kAccentChug,
    kBass,
    kTimpani,
    kHymn,
    kLead,
    kHeartbeat,
    kTremolo,
};

std::uint64_t Key(Instrument instrument, int pitch = 0, int steps = 0) {
    return instrument * 1'000'000u + static_cast<std::uint64_t>(pitch) * 1'000u + static_cast<std::uint64_t>(steps);
}

struct Composer {
    int sampleRate;
    NoteCache cache;
    std::vector<float>* stem = nullptr;

    void Play(int step, const std::vector<float>& note, float gain, std::size_t releaseFrames = 0) {
        MixInto(*stem, note, BossThemeStepFrame(step, sampleRate), gain, releaseFrames);
    }

    float StepSeconds() const {
        return 15.0f / static_cast<float>(kBossThemeBpm);
    }
};

bool IsFillBar(int bar) {
    return bar % 8 == 7;
}

// Drums, guitar and bass: everything the fight needs to sound like a boss fight on its own.
void ComposePulse(Composer& composer) {
    NoteCache& cache = composer.cache;
    const std::vector<float>& kick = cache.Get(Key(kKick), Kick());
    const std::vector<float>& snare = cache.Get(Key(kSnare), Snare());
    const std::vector<float>& hat = cache.Get(Key(kHiHat), HiHat());
    const std::vector<float>& crash = cache.Get(Key(kCrash), Crash());
    // A gallop: an eighth and two sixteenths on every beat, the run of the heavy metal of the eighties.
    constexpr std::array<int, 12> kGallop{0, 2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15};
    for (int bar = 0; bar < kBossThemeBars; ++bar) {
        const int start = bar * kStepsPerBar;
        const Chord& chord = kChords[static_cast<std::size_t>(bar)];
        const bool fill = IsFillBar(bar);
        if (bar % 4 == 0) {
            composer.Play(start, crash, 0.35f);
        }
        for (const int step : {0, 2, 6, 8, 10, 14}) {
            if (!(fill && step == 14)) {
                composer.Play(start + step, kick, 0.9f);
            }
        }
        composer.Play(start + 4, snare, 0.85f);
        if (fill) {
            // The last beat of each phrase rolls into the next one.
            constexpr std::array<float, 4> kRoll{0.45f, 0.55f, 0.65f, 0.8f};
            for (int i = 0; i < 4; ++i) {
                composer.Play(start + 12 + i, snare, kRoll[static_cast<std::size_t>(i)]);
            }
        } else {
            composer.Play(start + 12, snare, 0.85f);
        }
        for (int step = 0; step < kStepsPerBar; step += 2) {
            composer.Play(start + step, hat, step % 4 == 0 ? 0.35f : 0.25f);
        }
        const std::vector<float>& chug = cache.Get(Key(kChug, chord.root), Chug(chord.root, false));
        const std::vector<float>& accent = cache.Get(Key(kAccentChug, chord.root), Chug(chord.root, true));
        for (const int step : kGallop) {
            composer.Play(start + step, step == 0 ? accent : chug, step == 0 ? 0.7f : 0.6f);
        }
        const std::vector<float>& bass = cache.Get(Key(kBass, chord.root), Bass(chord.root));
        for (int step = 0; step < kStepsPerBar; step += 2) {
            composer.Play(start + step, bass, 0.35f);
        }
    }
}

// The hymn and the timpani: the same chords, held, under the gallop.
void ComposeHymn(Composer& composer) {
    NoteCache& cache = composer.cache;
    const float barSeconds = composer.StepSeconds() * static_cast<float>(kStepsPerBar);
    // Each chord holds a little past its bar, and fades out under the next one as it swells in.
    const float voiceSeconds = barSeconds + 0.1f;
    const auto release = static_cast<std::size_t>(0.2f * static_cast<float>(composer.sampleRate));
    for (int bar = 0; bar < kBossThemeBars; ++bar) {
        const int start = bar * kStepsPerBar;
        const Chord& chord = kChords[static_cast<std::size_t>(bar)];
        // A close voicing between A4 and G#5, high over the guitars where a choir sings, so the voices move by
        // small steps from chord to chord, and the root again an octave under.
        const std::array<int, 4> voices{InRange(chord.root % 12, 69), InRange(ThirdOf(chord) % 12, 69),
                                        InRange(FifthOf(chord) % 12, 69), InRange(chord.root % 12, 57)};
        for (const int pitch : voices) {
            composer.Play(start, cache.Get(Key(kHymn, pitch), HymnVoice(pitch, voiceSeconds)), 0.32f, release);
        }
        const std::vector<float>& timpani = cache.Get(Key(kTimpani, chord.root), Timpani(chord.root));
        composer.Play(start, timpani, 0.7f);
        composer.Play(start + 8, timpani, 0.55f);
        if (IsFillBar(bar)) {
            for (int i = 0; i < 4; ++i) {
                composer.Play(start + 12 + i, timpani, 0.35f + 0.08f * static_cast<float>(i));
            }
        }
    }
}

// Double kick on every sixteenth, a crash on every bar, and the lead over it all.
void ComposeWrath(Composer& composer) {
    NoteCache& cache = composer.cache;
    const std::vector<float>& kick = cache.Get(Key(kDoubleKick), DoubleKick());
    const std::vector<float>& crash = cache.Get(Key(kCrash), Crash());
    for (int step = 0; step < kLoopSteps; ++step) {
        composer.Play(step, kick, step % 4 == 0 ? 0.55f : 0.45f);
        if (step % kStepsPerBar == 0) {
            composer.Play(step, crash, 0.3f);
        }
    }
    const auto release = static_cast<std::size_t>(0.03f * static_cast<float>(composer.sampleRate));
    int step = 0;
    for (const MelodyNote& note : kMelody) {
        const float seconds = composer.StepSeconds() * static_cast<float>(note.steps);
        composer.Play(step, cache.Get(Key(kLead, note.pitch, note.steps), Lead(note.pitch, seconds)), 0.55f, release);
        step += note.steps;
    }
}

// A heart racing at the tempo of the fight, and the strings trembling on the chord's fifth and the half step
// above it.
void ComposeDesperation(Composer& composer) {
    NoteCache& cache = composer.cache;
    const std::vector<float>& heart = cache.Get(Key(kHeartbeat), Heartbeat());
    for (int bar = 0; bar < kBossThemeBars; ++bar) {
        const int start = bar * kStepsPerBar;
        const Chord& chord = kChords[static_cast<std::size_t>(bar)];
        for (int beat = 0; beat < kStepsPerBar; beat += 4) {
            // Lub on the beat, dub a sixteenth after, between the kicks.
            composer.Play(start + beat, heart, 0.9f);
            composer.Play(start + beat + 1, heart, 0.55f);
        }
        const int pitch = InRange(FifthOf(chord) % 12, 76);
        const std::vector<float>& stroke = cache.Get(Key(kTremolo, pitch), Tremolo(pitch));
        for (int step = 0; step < kStepsPerBar; ++step) {
            composer.Play(start + step, stroke, step % 4 == 0 ? 0.65f : 0.55f);
        }
    }
}

} // namespace

std::size_t BossThemeStepFrame(int step, int sampleRate) {
    // Worked out from the step every time, never added up, so no rounding drifts over the loop.
    const double frames = static_cast<double>(step) * static_cast<double>(sampleRate) * 15.0 / kBossThemeBpm;
    return static_cast<std::size_t>(std::llround(frames));
}

MusicTrack ComposeBossTheme(int sampleRate) {
    MusicTrack track;
    track.sampleRate = sampleRate;
    track.loopFrames = BossThemeStepFrame(kLoopSteps, sampleRate);
    Composer composer{.sampleRate = sampleRate, .cache = NoteCache{.sampleRate = sampleRate, .sounds = {}}};
    const auto compose = [&](MusicStem stem, void (*write)(Composer&)) {
        std::vector<float>& samples = track.stems[static_cast<std::size_t>(stem)];
        samples.assign(track.loopFrames, 0.0f);
        composer.stem = &samples;
        write(composer);
    };
    compose(MusicStem::Pulse, ComposePulse);
    compose(MusicStem::Hymn, ComposeHymn);
    compose(MusicStem::Wrath, ComposeWrath);
    compose(MusicStem::Desperation, ComposeDesperation);

    // One scale for all four layers, from the loudest moment of all of them together, so the balance between
    // them is kept and the mix never clips, whichever layers are up.
    float loudest = 0.0f;
    for (std::size_t i = 0; i < track.loopFrames; ++i) {
        float sum = 0.0f;
        for (const std::vector<float>& stem : track.stems) {
            sum += stem[i];
        }
        loudest = std::max(loudest, std::abs(sum));
    }
    const float scale = loudest > 0.0f ? kPeak / loudest : 0.0f;
    for (std::vector<float>& stem : track.stems) {
        for (float& sample : stem) {
            sample *= scale;
        }
    }
    return track;
}

} // namespace horde
