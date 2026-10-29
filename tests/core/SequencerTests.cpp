#include <vector>

#include <doctest/doctest.h>

#include "core/Sequencer.h"

TEST_CASE("a note's pitch doubles every twelve notes, from A4 at 440 Hz") {
    CHECK(horde::NoteHz(69) == doctest::Approx(440.0f));
    CHECK(horde::NoteHz(81) == doctest::Approx(880.0f));
    CHECK(horde::NoteHz(57) == doctest::Approx(220.0f));
    CHECK(horde::NoteHz(60) == doctest::Approx(261.63f).epsilon(0.001));
}

TEST_CASE("a note is mixed in at its frame, scaled, on top of what is already there") {
    std::vector<float> buffer(8, 0.1f);
    const std::vector<float> note{1.0f, 0.5f};
    horde::MixInto(buffer, note, 3, 0.5f);
    CHECK(buffer[2] == doctest::Approx(0.1f));
    CHECK(buffer[3] == doctest::Approx(0.6f));
    CHECK(buffer[4] == doctest::Approx(0.35f));
    CHECK(buffer[5] == doctest::Approx(0.1f));
}

TEST_CASE("a note that runs past the end of the loop wraps round to its start") {
    std::vector<float> buffer(4, 0.0f);
    const std::vector<float> note{1.0f, 2.0f, 3.0f};
    horde::MixInto(buffer, note, 3, 1.0f);
    CHECK(buffer[3] == doctest::Approx(1.0f));
    CHECK(buffer[0] == doctest::Approx(2.0f));
    CHECK(buffer[1] == doctest::Approx(3.0f));
    CHECK(buffer[2] == doctest::Approx(0.0f));

    // A start past the end lands where it would after that many loops.
    std::vector<float> again(4, 0.0f);
    horde::MixInto(again, std::vector<float>{1.0f}, 9, 1.0f);
    CHECK(again[1] == doctest::Approx(1.0f));
}

TEST_CASE("the release fades a note's last frames out to silence") {
    std::vector<float> buffer(6, 0.0f);
    const std::vector<float> note(6, 1.0f);
    horde::MixInto(buffer, note, 0, 1.0f, 3);
    CHECK(buffer[2] == doctest::Approx(1.0f));
    CHECK(buffer[3] == doctest::Approx(2.0f / 3.0f));
    CHECK(buffer[4] == doctest::Approx(1.0f / 3.0f));
    CHECK(buffer[5] == doctest::Approx(0.0f));
}
