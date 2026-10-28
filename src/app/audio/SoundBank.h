#pragma once

#include <array>

#include <raylib.h>

#include "core/SoundCue.h"

namespace horde {

/// The most copies of one sound that can play over each other.
inline constexpr int kMaxVoices = 8;

/// Every sound of the fight, synthesized once and loaded into raylib, with a few voices each so a sound can
/// play again before the last one has ended. The SMG fires ten times a second, and a shot lasts longer than
/// a tenth of a second.
struct SoundBank {
    struct Slot {
        std::array<Sound, kMaxVoices> voices{};
        int count = 0;
        int next = 0; ///< The voice the next play takes: the one that started longest ago.
    };
    std::array<Slot, kSoundCount> slots{};
    bool ready = false; ///< False when there is no audio device; then nothing plays and nothing fails.
};

/// Synthesizes every sound in `SoundLibrary` and loads it. Needs `InitAudioDevice` first.
[[nodiscard]] SoundBank LoadSoundBank();

/// Frees what `LoadSoundBank` loaded. Needs the audio device still open.
void UnloadSoundBank(SoundBank& bank);

/// Plays `cue` on the next voice of its sound, cutting off that voice's last play if it is still going.
void PlayCue(SoundBank& bank, const SoundCue& cue);

} // namespace horde
