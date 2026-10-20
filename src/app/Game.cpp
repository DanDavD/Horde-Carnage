#include "app/Game.h"

#include <cmath>

#include <raylib.h>

namespace horde {

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

constexpr Color kBackground{20, 18, 24, 255};
constexpr Color kBlood{220, 60, 50, 255};
constexpr Color kSoftText{170, 165, 180, 255};

void DrawCenteredText(const char* text, int y, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, (kScreenWidth - width) / 2, y, fontSize, color);
}

} // namespace

Game::Game() {
    // Vsync caps how often we render. How often the game simulates comes from FixedTimestep.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
}

Game::~Game() {
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose()) {
        const int steps = _timestep.Advance(GetFrameTime());
        for (int i = 0; i < steps; ++i) {
            Update(static_cast<float>(_timestep.StepSeconds()));
        }
        Draw(static_cast<float>(_timestep.Alpha()));
    }
}

void Game::Update(float dt) {
    _simulationTime += dt;
}

void Game::Draw(float alpha) const {
    BeginDrawing();
    ClearBackground(kBackground);

    // The title bobs with simulation time (interpolated), so the fixed-step loop is visible on screen.
    const double time = _simulationTime + alpha * _timestep.StepSeconds();
    const int bob = static_cast<int>(std::sin(time * 2.0) * 8.0);
    DrawCenteredText("HORDE CARNAGE", kScreenHeight / 2 - 60 + bob, 64, kBlood);
    DrawCenteredText("v" HORDE_VERSION " - project setup build. Press Esc to quit.", kScreenHeight / 2 + 30, 20,
                     kSoftText);

    DrawFPS(10, 10);
    EndDrawing();
}

} // namespace horde
