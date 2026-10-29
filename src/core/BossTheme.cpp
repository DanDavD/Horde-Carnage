#include "core/BossTheme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <map>
#include <vector>

#include "core/Sequencer.h"

namespace horde {

namespace {

constexpr int kLoopSteps = kBossThemeBars * kStepsPerBar;
// The loudest the layers ever get together, like the sound effects' peak.
constexpr float kPeak = 0.89f;
// A stinger plays alone, at the same peak as the theme with everything in.
constexpr float kStingerPeak = 0.89f;
constexpr float kVictorySeconds = 10.5f;
constexpr float kDefeatSeconds = 6.5f;

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

// A rest in a melody.
constexpr int kRest = 0;

// The Hook, a guitar in the register under the lead, so the opening of the fight has a tune of its own. Over the
// first phrase, a figure on each chord that climbs, answered by a held note; over the second, long notes on
// the chords' thirds and fifths, under the riffs; and in the last bar it hits with the band and holds its breath
// with it.
constexpr MelodyNote kHookMelody[] = {
    {62, 4}, {65, 2}, {64, 2}, {62, 2}, {57, 2}, {60, 4},  // Dm
    {62, 6}, {57, 2}, {62, 8},                             // Dm
    {65, 4}, {70, 2}, {69, 2}, {65, 2}, {62, 2}, {65, 4},  // Bb
    {67, 6}, {65, 2}, {62, 8},                             // Bb
    {64, 4}, {67, 2}, {65, 2}, {64, 2}, {60, 2}, {64, 4},  // C
    {69, 6}, {67, 2}, {64, 8},                             // C
    {61, 4}, {64, 2}, {62, 2}, {61, 2}, {57, 2}, {64, 4},  // A
    {69, 4}, {67, 2}, {64, 2}, {61, 8},                    // A, the band stopping under it
    {65, 8}, {69, 8},                                      // Dm
    {67, 8}, {64, 8},                                      // C
    {65, 8}, {62, 8},                                      // Bb
    {64, 8}, {61, 8},                                      // A
    {69, 16},                                              // Dm
    {72, 16},                                              // F
    {70, 8}, {67, 8},                                      // Gm
    {69, 3}, {67, 3}, {64, 2}, {kRest, 8},                 // A, on the band's three hits
};

constexpr int HookSteps() {
    int steps = 0;
    for (const MelodyNote& note : kHookMelody) {
        steps += note.steps;
    }
    return steps;
}
static_assert(HookSteps() == kLoopSteps, "the hook must fill the loop exactly");

// The second phrase answers three of its chords with power chords that walk down to the next one: E, D, C, B
// into B flat; A, C sharp, E, C sharp into D; C, B flat, A, A flat into G.
struct Riff {
    int bar;
    std::array<int, 4> roots;
};
constexpr std::array<Riff, 3> kRiffs{{{9, {52, 50, 48, 47}}, {11, {45, 49, 52, 49}}, {13, {48, 46, 45, 44}}}};

// The victory's fanfare, over B flat, C and D major: the theme's own VI and VII rising to a major tonic.
constexpr MelodyNote kFanfare[] = {
    {74, 4}, {78, 4}, {81, 4}, {86, 4}, // D
    {86, 6}, {84, 2}, {82, 8},          // Bb
    {84, 4}, {86, 4}, {88, 8},          // C
    {81, 2}, {86, 22},                  // D, held as the last chord rings
};

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

// A power chord hit hard and let ring a moment: the band's stabs when it stops.
SoundRecipe Stab(int root) {
    const float hz = NoteHz(root);
    constexpr float kTight = 180.0f;
    return Recipe(0.32f, 6.0f, 220 + static_cast<std::uint64_t>(root),
                  {{.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz, .decay = 0.25f, .lowPassFromHz = 6500.0f,
                    .lowPassToHz = 2500.0f, .highPassHz = kTight},
                   {.waveform = Waveform::Saw, .fromHz = hz * 1.007f, .toHz = hz * 1.007f, .gain = 0.8f,
                    .decay = 0.25f, .lowPassFromHz = 6500.0f, .lowPassToHz = 2500.0f, .highPassHz = kTight},
                   {.waveform = Waveform::Saw, .fromHz = hz * 1.4983f, .toHz = hz * 1.4983f, .gain = 0.8f,
                    .decay = 0.25f, .lowPassFromHz = 6500.0f, .lowPassToHz = 2500.0f, .highPassHz = kTight},
                   {.waveform = Waveform::Square, .fromHz = hz * 2.0f, .toHz = hz * 2.0f, .gain = 0.35f,
                    .decay = 0.25f, .lowPassFromHz = 6500.0f, .lowPassToHz = 2500.0f, .highPassHz = kTight}});
}

// A power chord left to ring out, for the last chord of a stinger. `slide` bends it as it dies: below 1, the
// strings go slack.
SoundRecipe Ring(int root, float seconds, float slide) {
    const float hz = NoteHz(root);
    constexpr float kTight = 180.0f;
    return Recipe(seconds, 6.0f, 240 + static_cast<std::uint64_t>(root),
                  {{.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz * slide, .decay = 1.6f,
                    .lowPassFromHz = 5500.0f, .lowPassToHz = 2200.0f, .highPassHz = kTight},
                   {.waveform = Waveform::Saw, .fromHz = hz * 1.007f, .toHz = hz * 1.007f * slide, .gain = 0.8f,
                    .decay = 1.6f, .lowPassFromHz = 5500.0f, .lowPassToHz = 2200.0f, .highPassHz = kTight},
                   {.waveform = Waveform::Saw, .fromHz = hz * 1.4983f, .toHz = hz * 1.4983f * slide, .gain = 0.8f,
                    .decay = 1.6f, .lowPassFromHz = 5500.0f, .lowPassToHz = 2200.0f, .highPassHz = kTight},
                   {.waveform = Waveform::Square, .fromHz = hz * 2.0f, .toHz = hz * 2.0f * slide, .gain = 0.35f,
                    .decay = 1.6f, .lowPassFromHz = 5500.0f, .lowPassToHz = 2200.0f, .highPassHz = kTight}});
}

// A tom: a short, pitched thump.
SoundRecipe Tom(float hz) {
    return Recipe(0.35f, 1.5f, 960 + static_cast<std::uint64_t>(hz),
                  {{.waveform = Waveform::Sine, .fromHz = hz * 1.3f, .toHz = hz, .decay = 0.18f, .sweepSeconds = 0.05f},
                   {.waveform = Waveform::Noise, .gain = 0.4f, .decay = 0.02f, .lowPassFromHz = 3000.0f,
                    .lowPassToHz = 3000.0f}});
}

// A gong: sines at a bell's out-of-tune partials, each dying at its own rate, over the strike.
SoundRecipe Gong(float hz) {
    return Recipe(5.0f, 1.2f, 950,
                  {{.waveform = Waveform::Sine, .fromHz = hz, .toHz = hz, .decay = 2.5f},
                   {.waveform = Waveform::Sine, .fromHz = hz * 2.76f, .toHz = hz * 2.76f, .gain = 0.8f, .decay = 1.8f},
                   {.waveform = Waveform::Sine, .fromHz = hz * 5.4f, .toHz = hz * 5.4f, .gain = 0.5f, .decay = 1.0f},
                   {.waveform = Waveform::Noise, .gain = 0.4f, .decay = 0.03f, .lowPassFromHz = 1200.0f,
                    .lowPassToHz = 1200.0f}});
}

// The Hook's guitar: two saws a little apart for width, an octave under for body, through less clip than the
// rhythm guitar so the tune stays clear, and darker than the Wrath's lead so the two never sound alike.
SoundRecipe HookGuitar(int pitch, float seconds) {
    const float hz = NoteHz(pitch);
    return Recipe(seconds, 3.5f, 900 + static_cast<std::uint64_t>(pitch),
                  {{.waveform = Waveform::Saw, .fromHz = hz, .toHz = hz, .attack = 0.005f, .decay = 1.2f,
                    .vibratoHz = 5.0f, .vibratoDepth = 0.004f, .lowPassFromHz = 3200.0f, .lowPassToHz = 3200.0f,
                    .highPassHz = 150.0f},
                   {.waveform = Waveform::Saw, .fromHz = hz * 1.005f, .toHz = hz * 1.005f, .gain = 0.8f,
                    .attack = 0.005f, .decay = 1.2f, .vibratoHz = 4.6f, .vibratoDepth = 0.004f,
                    .lowPassFromHz = 3200.0f, .lowPassToHz = 3200.0f, .highPassHz = 150.0f},
                   {.waveform = Waveform::Square, .fromHz = hz * 0.5f, .toHz = hz * 0.5f, .gain = 0.3f,
                    .attack = 0.005f, .decay = 1.2f, .lowPassFromHz = 1500.0f, .lowPassToHz = 1500.0f}});
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
    kStab,
    kRing,
    kTom,
    kGong,
    kHook,
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

std::size_t Frames(float seconds, int sampleRate) {
    return static_cast<std::size_t>(seconds * static_cast<float>(sampleRate));
}

// A gallop: an eighth and two sixteenths on every beat, the run of the heavy metal of the eighties.
constexpr std::array<int, 12> kGallop{0, 2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15};

// The last bar of each phrase: the band stops dead on three hits, a dotted eighth apart, holds its breath for
// a beat and a half, and the snare rolls over the last beat into the next phrase.
void ComposeBreak(Composer& composer, int start, const Chord& chord) {
    NoteCache& cache = composer.cache;
    const std::vector<float>& kick = cache.Get(Key(kKick), Kick());
    const std::vector<float>& snare = cache.Get(Key(kSnare), Snare());
    const std::vector<float>& stab = cache.Get(Key(kStab, chord.root), Stab(chord.root));
    const std::vector<float>& bass = cache.Get(Key(kBass, chord.root), Bass(chord.root));
    composer.Play(start, cache.Get(Key(kCrash), Crash()), 0.4f);
    for (const int step : {0, 3, 6}) {
        composer.Play(start + step, stab, 0.75f);
        composer.Play(start + step, kick, 0.9f);
        composer.Play(start + step, bass, 0.4f);
    }
    for (int i = 0; i < 4; ++i) {
        composer.Play(start + 12 + i, snare, 0.5f + 0.12f * static_cast<float>(i));
    }
}

// Four toms down the kit over the last beat of a bar.
void ComposeTomFill(Composer& composer, int start) {
    constexpr std::array<float, 4> kToms{196.0f, 165.0f, 131.0f, 110.0f};
    for (std::size_t i = 0; i < kToms.size(); ++i) {
        const auto hz = static_cast<int>(kToms[i]);
        composer.Play(start + 12 + static_cast<int>(i), composer.cache.Get(Key(kTom, hz), Tom(kToms[i])),
                      0.55f + 0.05f * static_cast<float>(i));
    }
}

// Drums, guitar and bass: everything the fight needs to sound like a boss fight on its own. The first phrase
// gallops under the Hook; the second turns to a thrash beat, a snare on every off beat, with power-chord riffs
// that walk into the next chord. Each phrase stops dead in its last bar, and rolls the toms halfway through.
void ComposePulse(Composer& composer) {
    NoteCache& cache = composer.cache;
    const std::vector<float>& kick = cache.Get(Key(kKick), Kick());
    const std::vector<float>& snare = cache.Get(Key(kSnare), Snare());
    const std::vector<float>& hat = cache.Get(Key(kHiHat), HiHat());
    const std::vector<float>& crash = cache.Get(Key(kCrash), Crash());
    for (int bar = 0; bar < kBossThemeBars; ++bar) {
        const int start = bar * kStepsPerBar;
        const Chord& chord = kChords[static_cast<std::size_t>(bar)];
        if (IsFillBar(bar)) {
            ComposeBreak(composer, start, chord);
            continue;
        }
        const bool thrash = bar >= kBossThemeBars / 2;
        const bool tomFill = bar % 8 == 3;
        if (bar % 4 == 0) {
            composer.Play(start, crash, 0.35f);
        }
        if (thrash) {
            for (const int step : {0, 4, 8, 12}) {
                composer.Play(start + step, kick, 0.9f);
            }
            for (const int step : {2, 6, 10, 14}) {
                if (!(tomFill && step == 14)) {
                    composer.Play(start + step, snare, 0.7f);
                }
            }
        } else {
            for (const int step : {0, 2, 6, 8, 10, 14}) {
                if (!(tomFill && step == 14)) {
                    composer.Play(start + step, kick, 0.9f);
                }
            }
            composer.Play(start + 4, snare, 0.85f);
            if (!tomFill) {
                composer.Play(start + 12, snare, 0.85f);
            }
        }
        for (int step = 0; step < kStepsPerBar; step += 2) {
            composer.Play(start + step, hat, step % 4 == 0 ? 0.35f : 0.25f);
        }
        if (tomFill) {
            ComposeTomFill(composer, start);
        }

        if (!thrash) {
            const std::vector<float>& chug = cache.Get(Key(kChug, chord.root), Chug(chord.root, false));
            const std::vector<float>& accent = cache.Get(Key(kAccentChug, chord.root), Chug(chord.root, true));
            for (const int step : kGallop) {
                composer.Play(start + step, step == 0 ? accent : chug, step == 0 ? 0.7f : 0.6f);
            }
            const std::vector<float>& bass = cache.Get(Key(kBass, chord.root), Bass(chord.root));
            for (int step = 0; step < kStepsPerBar; step += 2) {
                composer.Play(start + step, bass, 0.35f);
            }
            continue;
        }
        const Riff* riff = nullptr;
        for (const Riff& candidate : kRiffs) {
            if (candidate.bar == bar) {
                riff = &candidate;
            }
        }
        // Straight eighths, picked hard, the riff taking over the second half of its bar.
        for (int step = 0; step < kStepsPerBar; step += 2) {
            const int root = riff != nullptr && step >= 8 ? riff->roots[static_cast<std::size_t>((step - 8) / 2)]
                                                         : chord.root;
            const bool accent = step == 0 || (riff != nullptr && step >= 8);
            composer.Play(start + step,
                          cache.Get(Key(accent ? kAccentChug : kChug, root), Chug(root, accent)),
                          accent ? 0.7f : 0.6f);
            composer.Play(start + step, cache.Get(Key(kBass, root), Bass(root)), 0.35f);
        }
    }
}

// The Hook's melody, through the whole loop.
void ComposeHook(Composer& composer) {
    const auto release = static_cast<std::size_t>(0.03f * static_cast<float>(composer.sampleRate));
    int step = 0;
    for (const MelodyNote& note : kHookMelody) {
        if (note.pitch != kRest) {
            const float seconds = composer.StepSeconds() * static_cast<float>(note.steps);
            composer.Play(step,
                          composer.cache.Get(Key(kHook, note.pitch, note.steps), HookGuitar(note.pitch, seconds)),
                          0.55f, release);
        }
        step += note.steps;
    }
}

// A close voicing between A4 and G#5, high over the guitars where a choir sings, so the voices move by small
// steps from chord to chord, and the root again an octave under.
std::array<int, 4> HymnVoicing(const Chord& chord) {
    return {InRange(chord.root % 12, 69), InRange(ThirdOf(chord) % 12, 69), InRange(FifthOf(chord) % 12, 69),
            InRange(chord.root % 12, 57)};
}

void PlayHymnChord(Composer& composer, const Chord& chord, int step, float seconds, float gain,
                   std::size_t releaseFrames) {
    const int tenths = static_cast<int>(std::lround(seconds * 10.0f));
    for (const int pitch : HymnVoicing(chord)) {
        composer.Play(step, composer.cache.Get(Key(kHymn, pitch, tenths), HymnVoice(pitch, seconds)), gain,
                      releaseFrames);
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
        PlayHymnChord(composer, chord, start, voiceSeconds, 0.32f, release);
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
            composer.Play(start + beat, heart, 1.45f);
            composer.Play(start + beat + 1, heart, 0.9f);
        }
        const int pitch = InRange(FifthOf(chord) % 12, 76);
        const std::vector<float>& stroke = cache.Get(Key(kTremolo, pitch), Tremolo(pitch));
        for (int step = 0; step < kStepsPerBar; ++step) {
            composer.Play(start + step, stroke, step % 4 == 0 ? 1.1f : 0.9f);
        }
    }
}

// The victory: the gallop climbs B flat, C and D major under a fanfare, and the minor theme lands on a major
// chord that rings out under the choir. The rise from VI and VII to a major I is a classic heroic cadence, and
// the two chords come from the theme itself.
void ComposeVictory(Composer& composer) {
    NoteCache& cache = composer.cache;
    const std::vector<float>& kick = cache.Get(Key(kKick), Kick());
    const std::vector<float>& snare = cache.Get(Key(kSnare), Snare());
    const std::vector<float>& hat = cache.Get(Key(kHiHat), HiHat());
    const std::vector<float>& crash = cache.Get(Key(kCrash), Crash());
    constexpr std::array<Chord, 3> kRise{{{kD, Quality::Major}, {kBb, Quality::Major}, {kC, Quality::Major}}};
    const float barSeconds = composer.StepSeconds() * static_cast<float>(kStepsPerBar);
    const auto release = static_cast<std::size_t>(0.2f * static_cast<float>(composer.sampleRate));
    for (int bar = 0; bar < static_cast<int>(kRise.size()); ++bar) {
        const int start = bar * kStepsPerBar;
        const Chord& chord = kRise[static_cast<std::size_t>(bar)];
        const bool roll = bar == static_cast<int>(kRise.size()) - 1;
        composer.Play(start, crash, 0.4f);
        for (int step = 0; step < kStepsPerBar; step += 2) {
            if (step != 4 && step != 12 && !(roll && step >= 12)) {
                composer.Play(start + step, kick, 0.85f);
            }
            composer.Play(start + step, hat, 0.3f);
        }
        composer.Play(start + 4, snare, 0.85f);
        if (roll) {
            for (int i = 0; i < 4; ++i) {
                composer.Play(start + 12 + i, snare, 0.5f + 0.12f * static_cast<float>(i));
            }
        } else {
            composer.Play(start + 12, snare, 0.85f);
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
        PlayHymnChord(composer, chord, start, barSeconds + 0.1f, 0.3f, release);
        const std::vector<float>& timpani = cache.Get(Key(kTimpani, chord.root), Timpani(chord.root));
        composer.Play(start, timpani, 0.7f);
        if (roll) {
            for (int i = 0; i < 8; ++i) {
                composer.Play(start + 8 + i, timpani, 0.3f + 0.07f * static_cast<float>(i));
            }
        }
    }

    const int last = static_cast<int>(kRise.size()) * kStepsPerBar;
    const Chord tonic{kD, Quality::Major};
    composer.Play(last, kick, 1.0f);
    composer.Play(last, crash, 0.5f);
    composer.Play(last, cache.Get(Key(kTimpani, kD), Timpani(kD)), 1.0f);
    composer.Play(last, cache.Get(Key(kRing, kD, 1), Ring(kD, 4.5f, 1.0f)), 0.8f,
                  Frames(1.0f, composer.sampleRate));
    PlayHymnChord(composer, tonic, last, 5.0f, 0.32f, Frames(1.5f, composer.sampleRate));

    int step = 0;
    for (std::size_t i = 0; i < std::size(kFanfare); ++i) {
        const MelodyNote& note = kFanfare[i];
        const float seconds = composer.StepSeconds() * static_cast<float>(note.steps);
        // The last note is let go slowly, the rest are cut to their length.
        const bool held = i + 1 == std::size(kFanfare);
        composer.Play(step, cache.Get(Key(kLead, note.pitch, note.steps), Lead(note.pitch, seconds)), 0.6f,
                      Frames(held ? 0.6f : 0.03f, composer.sampleRate));
        step += note.steps;
    }
}

// The death: one blow, a gong, and a chord whose strings go slack as it dies, under a choir on a minor chord
// with the half step above its root; then the Desperation's heartbeat, three beats further and further apart,
// and nothing.
void ComposeDefeat(Composer& composer) {
    NoteCache& cache = composer.cache;
    const std::vector<float>& heart = cache.Get(Key(kHeartbeat), Heartbeat());
    const std::vector<float>& timpani = cache.Get(Key(kTimpani, kD), Timpani(kD));
    composer.Play(0, cache.Get(Key(kKick), Kick()), 1.0f);
    composer.Play(0, cache.Get(Key(kCrash), Crash()), 0.3f);
    composer.Play(0, timpani, 1.0f);
    composer.Play(0, cache.Get(Key(kGong), Gong(NoteHz(kD))), 0.8f);
    // Down a whole step over four seconds.
    composer.Play(0, cache.Get(Key(kRing, kD, 2), Ring(kD, 4.0f, 0.8909f)), 0.7f, Frames(1.0f, composer.sampleRate));
    for (const int pitch : {50, 62, 63, 65}) {
        composer.Play(0, cache.Get(Key(kHymn, pitch, 50), HymnVoice(pitch, 5.0f)), 0.4f,
                      Frames(1.5f, composer.sampleRate));
    }
    composer.Play(0, heart, 1.0f);
    composer.Play(12, heart, 0.7f);
    composer.Play(12, timpani, 0.45f);
    composer.Play(26, heart, 0.45f);
}

// Scales `samples` so its loudest sample is `peak`.
void Normalize(std::vector<float>& samples, float peak) {
    float loudest = 0.0f;
    for (const float sample : samples) {
        loudest = std::max(loudest, std::abs(sample));
    }
    const float scale = loudest > 0.0f ? peak / loudest : 0.0f;
    for (float& sample : samples) {
        sample *= scale;
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
    compose(MusicStem::Hook, ComposeHook);

    // The stingers are long enough that no note of theirs wraps round to their start.
    track.victory.assign(Frames(kVictorySeconds, sampleRate), 0.0f);
    composer.stem = &track.victory;
    ComposeVictory(composer);
    Normalize(track.victory, kStingerPeak);
    track.defeat.assign(Frames(kDefeatSeconds, sampleRate), 0.0f);
    composer.stem = &track.defeat;
    ComposeDefeat(composer);
    Normalize(track.defeat, kStingerPeak);

    // One scale for all the layers, from the loudest moment of all of them together, so the balance between
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
