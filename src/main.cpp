#include "app/Game.h"

#include <memory>
#include <string_view>

int main(int argc, char** argv) {
    // `--windowed` opens a window that can be resized instead of the whole screen, for recording and
    // debugging. Anything else is ignored.
    bool windowed = false;
    for (int i = 1; i < argc; ++i) {
        windowed = windowed || std::string_view(argv[i]) == "--windowed";
    }
    // On the heap: the game holds the whole fight in fixed pools (the world, 512 particles, every sound's
    // voices), over 60 KB, and a browser gives the program only 64 KB of stack.
    const auto game = std::make_unique<horde::Game>(windowed);
    game->Run();
    return 0;
}
