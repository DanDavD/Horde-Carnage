#pragma once

#include <cstddef>

#include "core/MusicTrack.h"
#include "core/Synth.h"

namespace horde {

/// The tempo of the boss theme. At 150 beats a minute a sixteenth note lasts exactly a tenth of a second.
inline constexpr int kBossThemeBpm = 150;
/// The length of the loop: two phrases of eight bars, in 4/4, 25.6 seconds.
inline constexpr int kBossThemeBars = 16;
/// The theme is written in sixteenth notes.
inline constexpr int kStepsPerBar = 16;

/// The frame of the loop where sixteenth note `step` starts, at `sampleRate`.
[[nodiscard]] std::size_t BossThemeStepFrame(int step, int sampleRate);

/// Composes the Matriarch's theme: a fast metal gallop in D minor, in five layers written over the same
/// sixteen bars (see `MusicStem`), and the two stingers that end a fight (see `MusicStinger`). Every note is
/// synthesized in code, like the sound effects. The layers are scaled together so that all of them at full
/// volume never clip. It allocates about 25 MB at 44.1 kHz and takes a moment, so the game composes it once
/// while loading. The same rate always gives the same samples.
[[nodiscard]] MusicTrack ComposeBossTheme(int sampleRate = kSynthSampleRate);

} // namespace horde
