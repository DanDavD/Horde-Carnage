#pragma once

#include <string>
#include <string_view>

namespace horde {

/// How much the screen shakes on a hit.
enum class ShakeLevel { Off, Low, Full };

/// How often the game draws a frame. The fight always steps 60 times a second whatever this is.
enum class FrameRateCap { VSync, Fps60, Fps120, Fps144, Unlimited };

/// What the player can change in the settings screen. Kept between runs.
struct Settings {
    int masterVolume = 10; ///< From 0 (silent) to `kMaxVolume`.
    int musicVolume = 10;  ///< The music alone, under the master volume, from 0 to `kMaxVolume`.
    int mouseSensitivity = 5; ///< How far the crosshair moves for the mouse's movement, 1 to `kMaxSensitivity`.
    int stickSensitivity = 5; ///< How fast the gamepad's aim turns toward the right stick, 1 to `kMaxSensitivity`.
    ShakeLevel shake = ShakeLevel::Full;
    bool fullscreen = true;
    FrameRateCap frameRate = FrameRateCap::VSync;
};

inline constexpr int kMaxVolume = 10;
inline constexpr int kMinSensitivity = 1;
inline constexpr int kMaxSensitivity = 10;

/// One line of the settings screen that holds a value.
enum class Setting { MasterVolume, MusicVolume, MouseSensitivity, StickSensitivity, ScreenShake, Display, FrameRate };

/// Changes `setting` by `step` (-1 or 1). The volume stops at both ends, so holding a key never jumps from
/// full to silent; the other settings wrap round, since each is a short list of choices. Returns whether
/// anything changed.
bool AdjustSetting(Settings& settings, Setting setting, int step);

/// The gain for raylib's master volume. The level is squared, because the ear hears loudness roughly on a
/// log scale: with a straight line, the top half of the slider would sound almost the same. Full volume is
/// under 1, so the fight's loudest moments, mixed together, do not clip.
[[nodiscard]] float MasterGain(const Settings& settings);

/// The gain of the music under the master volume: squared like it, from 0 to 1 at full, so the master volume's
/// headroom is not taken twice.
[[nodiscard]] float MusicGain(const Settings& settings);

/// What the mouse's movement is multiplied by: 1 at the middle level 5, as the cursor moves; each level
/// away from it is the same step on a log scale, from about a third at 1 to 4 at 10.
[[nodiscard]] float MouseAimScale(const Settings& settings);

/// How fast the gamepad's aim may turn toward the right stick, in radians a second: about 1440 degrees
/// a second at the middle level 5, a sweep of the whole circle in a quarter of a second, down to about 160
/// at 1 for fine aim and, at 10, so fast it is the stick's own direction at once.
[[nodiscard]] float StickTurnRadiansPerSecond(const Settings& settings);

/// How much of the screen shake is shown: 0, a half, or all of it.
[[nodiscard]] float ShakeAmount(ShakeLevel level);

/// The frame cap for raylib's `SetTargetFPS`: 0 for none, as with vsync or Unlimited.
[[nodiscard]] int TargetFramesPerSecond(FrameRateCap cap);

/// The value of `setting` as the settings screen shows it, in capitals: "FULL", "WINDOWED", "144"...
/// The volume is drawn as a row of marks instead, so its text is only the number.
[[nodiscard]] std::string SettingValueText(const Settings& settings, Setting setting);

/// `settings` as text, one `key=value` per line, to keep in a file or the browser's storage.
[[nodiscard]] std::string FormatSettings(const Settings& settings);

/// Reads what `FormatSettings` wrote. Anything missing or unknown keeps its default and a volume out of range
/// is brought back into it, so an old or hand-edited file never stops the game from starting.
[[nodiscard]] Settings ParseSettings(std::string_view text);

} // namespace horde
