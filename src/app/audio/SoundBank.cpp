#include "app/audio/SoundBank.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "core/SoundLibrary.h"
#include "core/Synth.h"

namespace horde {

namespace {

// Sounds that come many times a second get the most voices; the rest seldom overlap with themselves.
int VoicesFor(SoundId id) {
    switch (id) {
    case SoundId::PistolShot:
    case SoundId::SmgShot:
        return kMaxVoices;
    case SoundId::BossFlesh:
    case SoundId::ShellPing:
    case SoundId::ShieldKnock:
    case SoundId::Spit:
        return 5;
    default:
        return 2;
    }
}

} // namespace

SoundBank LoadSoundBank() {
    SoundBank bank;
    if (!IsAudioDeviceReady()) {
        return bank;
    }
    for (std::size_t i = 0; i < kSoundCount; ++i) {
        const auto id = static_cast<SoundId>(i);
        std::vector<float> samples = Synthesize(RecipeFor(id));
        // raylib copies the samples into its own buffer, so the vector can go once the sound is loaded.
        const Wave wave{.frameCount = static_cast<unsigned int>(samples.size()),
                        .sampleRate = static_cast<unsigned int>(kSynthSampleRate),
                        .sampleSize = 32,
                        .channels = 1,
                        .data = samples.data()};
        SoundBank::Slot& slot = bank.slots[i];
        slot.voices[0] = LoadSoundFromWave(wave);
        slot.count = VoicesFor(id);
        // The other voices share the first one's samples instead of holding copies.
        for (int voice = 1; voice < slot.count; ++voice) {
            slot.voices[static_cast<std::size_t>(voice)] = LoadSoundAlias(slot.voices[0]);
        }
    }
    bank.ready = true;
    return bank;
}

void UnloadSoundBank(SoundBank& bank) {
    if (!bank.ready) {
        return;
    }
    for (SoundBank::Slot& slot : bank.slots) {
        for (int voice = slot.count - 1; voice >= 1; --voice) {
            UnloadSoundAlias(slot.voices[static_cast<std::size_t>(voice)]);
        }
        UnloadSound(slot.voices[0]);
        slot.count = 0;
    }
    bank.ready = false;
}

void PlayCue(SoundBank& bank, const SoundCue& cue) {
    if (!bank.ready) {
        return;
    }
    SoundBank::Slot& slot = bank.slots[static_cast<std::size_t>(cue.id)];
    const Sound& voice = slot.voices[static_cast<std::size_t>(slot.next)];
    slot.next = (slot.next + 1) % slot.count;
    SetSoundVolume(voice, std::clamp(cue.volume, 0.0f, 1.0f));
    SetSoundPitch(voice, cue.pitch);
    // raylib's pan runs from 1 on the left to 0 on the right, the other way round from the cue's.
    SetSoundPan(voice, 0.5f - 0.5f * cue.pan);
    PlaySound(voice);
}

} // namespace horde
