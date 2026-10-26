#include "app/Game.h"

#include <string_view>

int main(int argc, char** argv) {
    // `--windowed` opens a window that can be resized instead of the whole screen, for recording and
    // debugging. Anything else is ignored.
    bool windowed = false;
    for (int i = 1; i < argc; ++i) {
        windowed = windowed || std::string_view(argv[i]) == "--windowed";
    }
    horde::Game game(windowed);
    game.Run();
    return 0;
}
