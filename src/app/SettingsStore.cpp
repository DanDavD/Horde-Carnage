#include "app/SettingsStore.h"

#include <cstdlib>
#include <string>

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace horde {

namespace {

#ifdef __EMSCRIPTEN__
// A browser keeps no files between visits, so the settings go in the page's local storage. Storage can be
// blocked, for example for a game inside another site's frame, and then reading and writing it throws.
EM_JS_DEPS(SettingsStore, "$stringToNewUTF8,$UTF8ToString");

EM_JS(char*, ReadStoredSettings, (), {
    let text = "";
    try {
        text = localStorage.getItem("horde-carnage-settings") || "";
    } catch (e) {
    }
    return stringToNewUTF8(text);
});

EM_JS(void, WriteStoredSettings, (const char* text), {
    try {
        localStorage.setItem("horde-carnage-settings", UTF8ToString(text));
    } catch (e) {
    }
});
#else
// Next to the game, not in the folder it was started from, so a shortcut finds the same file. The game is a
// zip to unpack anywhere, so its folder is the place a player would look.
std::string SettingsPath() {
    return std::string(GetApplicationDirectory()) + "settings.txt";
}
#endif

} // namespace

Settings LoadSettings() {
#ifdef __EMSCRIPTEN__
    char* text = ReadStoredSettings();
    const Settings settings = ParseSettings(text);
    std::free(text);
    return settings;
#else
    const std::string path = SettingsPath();
    if (!FileExists(path.c_str())) {
        return Settings{};
    }
    char* text = LoadFileText(path.c_str());
    const Settings settings = text != nullptr ? ParseSettings(text) : Settings{};
    UnloadFileText(text);
    return settings;
#endif
}

void SaveSettings(const Settings& settings) {
    std::string text = FormatSettings(settings);
#ifdef __EMSCRIPTEN__
    WriteStoredSettings(text.c_str());
#else
    // raylib logs a warning on failure; there is nothing else to do about it.
    SaveFileText(SettingsPath().c_str(), text.data());
#endif
}

} // namespace horde
