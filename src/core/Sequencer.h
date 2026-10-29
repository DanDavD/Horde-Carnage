#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace horde {

/// The pitch of a MIDI note number, in Hz, in equal temperament: 69 is A4 at 440 Hz, and twelve notes up is
/// twice the pitch.
[[nodiscard]] float NoteHz(int midiNote);

/// Adds the samples of `note` into `buffer`, from `startFrame` on, scaled by `gain`. Whatever runs past the
/// end of the buffer wraps round to its start, so the last notes of a loop ring on into its first bar and the
/// seam is never heard. The last `releaseFrames` fade out in a straight line, so a note cut to its length ends
/// without a click.
void MixInto(std::vector<float>& buffer, std::span<const float> note, std::size_t startFrame, float gain,
             std::size_t releaseFrames = 0);

} // namespace horde
