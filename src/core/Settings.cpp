#include "core/Settings.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>

namespace horde {

namespace {

// Under full volume, so the fight's loudest moments, mixed together, do not clip.
constexpr float kFullGain = 0.8f;

// The names used in the saved text, in the order of each enum.
constexpr std::array<std::string_view, 3> kShakeNames{"off", "low", "full"};
constexpr std::array<std::string_view, 5> kFrameRateNames{"vsync", "60", "120", "144", "unlimited"};

// The same names, as the settings screen shows them.
constexpr std::array<const char*, 3> kShakeLabels{"OFF", "LOW", "FULL"};
constexpr std::array<const char*, 5> kFrameRateLabels{"VSYNC", "60", "120", "144", "UNLIMITED"};

// `value` moved by `step` through `count` choices, round past either end.
template <typename Enum>
Enum Cycled(Enum value, int step, int count) {
    const int index = ((static_cast<int>(value) + step) % count + count) % count;
    return static_cast<Enum>(index);
}

template <typename Enum, std::size_t N>
bool ParseName(std::string_view text, const std::array<std::string_view, N>& names, Enum& value) {
    for (std::size_t i = 0; i < N; ++i) {
        if (names[i] == text) {
            value = static_cast<Enum>(i);
            return true;
        }
    }
    return false;
}

std::string_view Trimmed(std::string_view text) {
    constexpr std::string_view kSpace = " \t\r";
    const std::size_t first = text.find_first_not_of(kSpace);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kSpace);
    return text.substr(first, last - first + 1);
}

// A whole number from 0 to `kMaxVolume`, brought back into range; anything else leaves `volume` as it was.
void ParseVolume(std::string_view value, int& volume) {
    int read = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), read);
    if (error == std::errc{} && end == value.data() + value.size()) {
        volume = std::clamp(read, 0, kMaxVolume);
    }
}

// From 0 to 1 at full, squared: the ear hears loudness on a log scale.
float VolumeCurve(int volume) {
    const float level = static_cast<float>(std::clamp(volume, 0, kMaxVolume)) / kMaxVolume;
    return level * level;
}

void ParseLine(std::string_view key, std::string_view value, Settings& settings) {
    if (key == "volume") {
        ParseVolume(value, settings.masterVolume);
    } else if (key == "music") {
        ParseVolume(value, settings.musicVolume);
    } else if (key == "shake") {
        ParseName(value, kShakeNames, settings.shake);
    } else if (key == "display") {
        if (value == "fullscreen" || value == "windowed") {
            settings.fullscreen = value == "fullscreen";
        }
    } else if (key == "framerate") {
        ParseName(value, kFrameRateNames, settings.frameRate);
    }
}

} // namespace

bool AdjustSetting(Settings& settings, Setting setting, int step) {
    const Settings before = settings;
    switch (setting) {
    case Setting::MasterVolume:
        settings.masterVolume = std::clamp(settings.masterVolume + step, 0, kMaxVolume);
        return settings.masterVolume != before.masterVolume;
    case Setting::MusicVolume:
        settings.musicVolume = std::clamp(settings.musicVolume + step, 0, kMaxVolume);
        return settings.musicVolume != before.musicVolume;
    case Setting::ScreenShake:
        settings.shake = Cycled(settings.shake, step, static_cast<int>(kShakeNames.size()));
        return settings.shake != before.shake;
    case Setting::Display:
        // Two choices: any step flips it.
        settings.fullscreen = step % 2 != 0 ? !settings.fullscreen : settings.fullscreen;
        return settings.fullscreen != before.fullscreen;
    case Setting::FrameRate:
        settings.frameRate = Cycled(settings.frameRate, step, static_cast<int>(kFrameRateNames.size()));
        return settings.frameRate != before.frameRate;
    }
    return false;
}

float MasterGain(const Settings& settings) {
    return kFullGain * VolumeCurve(settings.masterVolume);
}

float MusicGain(const Settings& settings) {
    return VolumeCurve(settings.musicVolume);
}

float ShakeAmount(ShakeLevel level) {
    switch (level) {
    case ShakeLevel::Off:
        return 0.0f;
    case ShakeLevel::Low:
        return 0.5f;
    case ShakeLevel::Full:
        return 1.0f;
    }
    return 1.0f;
}

int TargetFramesPerSecond(FrameRateCap cap) {
    switch (cap) {
    case FrameRateCap::Fps60:
        return 60;
    case FrameRateCap::Fps120:
        return 120;
    case FrameRateCap::Fps144:
        return 144;
    case FrameRateCap::VSync:
    case FrameRateCap::Unlimited:
        break;
    }
    return 0;
}

std::string SettingValueText(const Settings& settings, Setting setting) {
    switch (setting) {
    case Setting::MasterVolume:
        return std::to_string(settings.masterVolume);
    case Setting::MusicVolume:
        return std::to_string(settings.musicVolume);
    case Setting::ScreenShake:
        return kShakeLabels[static_cast<std::size_t>(settings.shake)];
    case Setting::Display:
        return settings.fullscreen ? "FULLSCREEN" : "WINDOWED";
    case Setting::FrameRate:
        return kFrameRateLabels[static_cast<std::size_t>(settings.frameRate)];
    }
    return {};
}

std::string FormatSettings(const Settings& settings) {
    std::string text;
    text += "volume=" + std::to_string(settings.masterVolume) + "\n";
    text += "music=" + std::to_string(settings.musicVolume) + "\n";
    text += "shake=" + std::string(kShakeNames[static_cast<std::size_t>(settings.shake)]) + "\n";
    text += std::string("display=") + (settings.fullscreen ? "fullscreen" : "windowed") + "\n";
    text += "framerate=" + std::string(kFrameRateNames[static_cast<std::size_t>(settings.frameRate)]) + "\n";
    return text;
}

Settings ParseSettings(std::string_view text) {
    Settings settings;
    while (!text.empty()) {
        const std::size_t end = text.find('\n');
        const std::string_view line = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        const std::size_t equals = line.find('=');
        if (equals != std::string_view::npos) {
            ParseLine(Trimmed(line.substr(0, equals)), Trimmed(line.substr(equals + 1)), settings);
        }
    }
    return settings;
}

} // namespace horde
