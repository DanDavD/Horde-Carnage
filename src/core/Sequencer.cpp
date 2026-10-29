#include "core/Sequencer.h"

#include <algorithm>
#include <cmath>

namespace horde {

float NoteHz(int midiNote) {
    return 440.0f * std::pow(2.0f, static_cast<float>(midiNote - 69) / 12.0f);
}

void MixInto(std::vector<float>& buffer, std::span<const float> note, std::size_t startFrame, float gain,
             std::size_t releaseFrames) {
    if (buffer.empty()) {
        return;
    }
    const std::size_t release = std::min(releaseFrames, note.size());
    std::size_t at = startFrame % buffer.size();
    for (std::size_t i = 0; i < note.size(); ++i) {
        const std::size_t fromEnd = note.size() - 1 - i;
        const float tail = fromEnd < release ? static_cast<float>(fromEnd) / static_cast<float>(release) : 1.0f;
        buffer[at] += note[i] * gain * tail;
        at = at + 1 == buffer.size() ? 0 : at + 1;
    }
}

} // namespace horde
