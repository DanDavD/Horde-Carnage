#include <doctest/doctest.h>

#include "core/Settings.h"

using horde::FrameRateCap;
using horde::Setting;
using horde::Settings;
using horde::ShakeLevel;

TEST_CASE("the defaults are full volume, full shake, fullscreen and vsync") {
    const Settings settings;
    CHECK(settings.masterVolume == horde::kMaxVolume);
    CHECK(settings.shake == ShakeLevel::Full);
    CHECK(settings.fullscreen);
    CHECK(settings.frameRate == FrameRateCap::VSync);
}

TEST_CASE("the volume stops at both ends instead of wrapping") {
    Settings settings;
    CHECK_FALSE(horde::AdjustSetting(settings, Setting::MasterVolume, 1));
    CHECK(settings.masterVolume == 10);
    CHECK(horde::AdjustSetting(settings, Setting::MasterVolume, -1));
    CHECK(settings.masterVolume == 9);
    for (int i = 0; i < 20; ++i) {
        horde::AdjustSetting(settings, Setting::MasterVolume, -1);
    }
    CHECK(settings.masterVolume == 0);
    CHECK_FALSE(horde::AdjustSetting(settings, Setting::MasterVolume, -1));
}

TEST_CASE("the choices wrap round at both ends") {
    Settings settings;
    CHECK(horde::AdjustSetting(settings, Setting::ScreenShake, 1));
    CHECK(settings.shake == ShakeLevel::Off);
    horde::AdjustSetting(settings, Setting::ScreenShake, -1);
    CHECK(settings.shake == ShakeLevel::Full);

    horde::AdjustSetting(settings, Setting::FrameRate, -1);
    CHECK(settings.frameRate == FrameRateCap::Unlimited);
    horde::AdjustSetting(settings, Setting::FrameRate, 1);
    CHECK(settings.frameRate == FrameRateCap::VSync);

    CHECK(horde::AdjustSetting(settings, Setting::Display, 1));
    CHECK_FALSE(settings.fullscreen);
    CHECK(horde::AdjustSetting(settings, Setting::Display, -1));
    CHECK(settings.fullscreen);
}

TEST_CASE("the volume curve is silent at 0, under 1 at full, and quieter than a straight line in between") {
    Settings settings;
    settings.masterVolume = 0;
    CHECK(horde::MasterGain(settings) == 0.0f);
    settings.masterVolume = 10;
    const float full = horde::MasterGain(settings);
    CHECK(full > 0.5f);
    CHECK(full < 1.0f);
    settings.masterVolume = 5;
    CHECK(horde::MasterGain(settings) == doctest::Approx(full * 0.25f));
}

TEST_CASE("the music has a volume of its own, stopping at both ends, squared like the master") {
    Settings settings;
    CHECK(settings.musicVolume == horde::kMaxVolume);
    CHECK(horde::MusicGain(settings) == doctest::Approx(1.0f));
    CHECK_FALSE(horde::AdjustSetting(settings, Setting::MusicVolume, 1));
    CHECK(horde::AdjustSetting(settings, Setting::MusicVolume, -5));
    CHECK(settings.musicVolume == 5);
    CHECK(horde::MusicGain(settings) == doctest::Approx(0.25f));
    // The master volume is left alone.
    CHECK(settings.masterVolume == horde::kMaxVolume);
    CHECK(horde::AdjustSetting(settings, Setting::MusicVolume, -20));
    CHECK(settings.musicVolume == 0);
    CHECK(horde::MusicGain(settings) == 0.0f);
    CHECK(horde::SettingValueText(settings, Setting::MusicVolume) == "0");
}

TEST_CASE("the shake and the frame rate turn into numbers for the game") {
    CHECK(horde::ShakeAmount(ShakeLevel::Off) == 0.0f);
    CHECK(horde::ShakeAmount(ShakeLevel::Low) > 0.0f);
    CHECK(horde::ShakeAmount(ShakeLevel::Low) < 1.0f);
    CHECK(horde::ShakeAmount(ShakeLevel::Full) == 1.0f);
    CHECK(horde::TargetFramesPerSecond(FrameRateCap::VSync) == 0);
    CHECK(horde::TargetFramesPerSecond(FrameRateCap::Fps60) == 60);
    CHECK(horde::TargetFramesPerSecond(FrameRateCap::Fps120) == 120);
    CHECK(horde::TargetFramesPerSecond(FrameRateCap::Fps144) == 144);
    CHECK(horde::TargetFramesPerSecond(FrameRateCap::Unlimited) == 0);
}

TEST_CASE("each value reads in capitals on the settings screen") {
    Settings settings{.masterVolume = 7, .shake = ShakeLevel::Low, .fullscreen = false,
                      .frameRate = FrameRateCap::Fps144};
    CHECK(horde::SettingValueText(settings, Setting::MasterVolume) == "7");
    CHECK(horde::SettingValueText(settings, Setting::ScreenShake) == "LOW");
    CHECK(horde::SettingValueText(settings, Setting::Display) == "WINDOWED");
    CHECK(horde::SettingValueText(settings, Setting::FrameRate) == "144");
}

TEST_CASE("settings written as text read back the same") {
    const Settings settings{.masterVolume = 3, .musicVolume = 6, .shake = ShakeLevel::Off, .fullscreen = false,
                            .frameRate = FrameRateCap::Unlimited};
    const std::string text = horde::FormatSettings(settings);
    CHECK(text == "volume=3\nmusic=6\nshake=off\ndisplay=windowed\nframerate=unlimited\n");
    const Settings read = horde::ParseSettings(text);
    CHECK(read.masterVolume == 3);
    CHECK(read.musicVolume == 6);
    CHECK(read.shake == ShakeLevel::Off);
    CHECK_FALSE(read.fullscreen);
    CHECK(read.frameRate == FrameRateCap::Unlimited);
}

TEST_CASE("a damaged or hand-edited file keeps the defaults for what it gets wrong") {
    // Windows line ends and spaces are fine; a volume out of range is brought back; unknown keys, unknown
    // values, lines without '=' and numbers with junk after them are skipped.
    const Settings read = horde::ParseSettings(" volume = 25 \r\nshake=wobbly\nmusic=on\nbass=11\ngarbage\n"
                                               "framerate=144\r\ndisplay=windowed");
    CHECK(read.masterVolume == 10);
    CHECK(read.musicVolume == 10);
    CHECK(read.shake == ShakeLevel::Full);
    CHECK(read.frameRate == FrameRateCap::Fps144);
    CHECK_FALSE(read.fullscreen);

    CHECK(horde::ParseSettings("volume=-4").masterVolume == 0);
    CHECK(horde::ParseSettings("volume=4x").masterVolume == 10);
    CHECK(horde::ParseSettings("").masterVolume == 10);
    CHECK(horde::ParseSettings("music=-3").musicVolume == 0);
    // A file saved before the music had a volume reads with the music at full.
    CHECK(horde::ParseSettings("volume=4\nshake=low\n").musicVolume == 10);
}
