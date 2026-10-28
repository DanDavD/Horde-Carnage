#pragma once

#include "core/Settings.h"

namespace horde {

/// The settings saved by the last run, or the defaults when there are none. On the desktop they are in
/// `settings.txt` next to the game; in a browser, in the page's local storage.
[[nodiscard]] Settings LoadSettings();

/// Keeps `settings` for the next run. If they cannot be written (a folder the game may not write to, or a
/// browser that blocks storage), the game goes on with them for this run only.
void SaveSettings(const Settings& settings);

} // namespace horde
