#include "app/Game.h"

#include <algorithm>
#include <random>

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"
#include "app/render/ArenaDraw.h"
#include "app/render/DebugOverlay.h"
#include "app/render/EndScreen.h"
#include "app/render/Hud.h"
#include "app/render/Palette.h"
#include "app/render/View.h"
#include "app/render/WorldDraw.h"
#include "core/ImpactFeel.h"
#include "core/ScreenFit.h"

namespace horde {

// Every launch starts from a seed the system picks, so no two runs of the game repeat the same fights.
Game::Game(bool windowed)
    : _session{StartSession(kBossArena, kViewSize, std::random_device{}())},
      _particles{SeedParticleEmitter(std::random_device{}())} {
    // Vsync caps how often we render, at the monitor's own rate. How often the game simulates comes from
    // FixedTimestep.
#ifdef __EMSCRIPTEN__
    (void)windowed;
    // In a browser the canvas follows the page's size, so going fullscreen on itch.io fills the screen.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
    // Setting a minimum size makes raylib fit the canvas to the page once, right away.
    SetWindowMinSize(kScreenWidth / 4, kScreenHeight / 4);
#else
    // Fullscreen, at the monitor's own resolution: a borderless window over the whole monitor, so switching
    // to another program is instant and the resolution of the desktop is not changed. `--windowed` gives a
    // window that can be resized instead, for recording and debugging.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | (windowed ? FLAG_WINDOW_RESIZABLE : 0));
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
    if (!windowed) {
        ToggleBorderlessWindowed();
    }
#endif
    // The default font, which the Debug overlay uses, is drawn scaled with the rest of the picture; smoothed,
    // it scales evenly.
    SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_BILINEAR);
    _art = LoadArt(kBossArena);
}

Game::~Game() {
    UnloadArt(_art);
    CloseWindow();
}

void Game::Run() {
#ifdef __EMSCRIPTEN__
    // A browser tab cannot be held in a loop: the page calls one frame at a time, at its own refresh rate.
    // With the last argument set, this never returns, so `this` stays alive for the life of the page.
    emscripten_set_main_loop_arg([](void* game) { static_cast<Game*>(game)->Frame(); }, this, 0, true);
#else
    while (!WindowShouldClose()) {
        Frame();
    }
#endif
}

void Game::Frame() {
    // Key presses are read once per frame, not per fixed step: a frame with no step would miss them.
    if (ReadRestartPressed() && TryRestart(_session)) {
        ClearParticles(_particles);
    }
    CollectButtonPresses(_pendingPresses);

    // The crosshair replaces the system cursor while playing. On the end screens the cursor is back.
    SetCursorHidden(_session.state == SessionState::Playing);
#ifndef NDEBUG
    if (IsKeyPressed(KEY_F1)) {
        _showDebugOverlay = !_showDebugOverlay;
    }
    // Breaks the shield at once, to reach the exposed window without the right gun.
    if (IsKeyPressed(KEY_F2) && _session.state == SessionState::Playing) {
        BreakBossShield(_session.world);
    }
    // Skips ahead: the transformation in the first phase, the frenzy in the second.
    if (IsKeyPressed(KEY_F3) && _session.state == SessionState::Playing) {
        if (_session.world.boss.phase == BossPhase::One) {
            TransformBoss(_session.world);
        } else {
            FrenzyBoss(_session.world);
        }
    }
    // Skips to the victory screen without emptying both bars. The next step ends the fight.
    if (IsKeyPressed(KEY_F4) && _session.state == SessionState::Playing) {
        DefeatBoss(_session.world);
    }
#endif
    // A hit stop slows the fight's clock, so the world takes fewer of its fixed steps. Every step is still the
    // same length, so the rules play out exactly as at full speed, only slower to watch.
    const float frameSeconds = GetFrameTime();
    const float timeScale = TimeScale(_hitStop);
    // Existing particles advance first, scaled by the hit stop so shards and casings hang in slow motion with
    // the fight, while any burst spawned in this frame's steps is drawn once right where it started.
    StepParticles(_particles.particles, _session.world.arena, std::min(frameSeconds, 0.25f) * timeScale);
    const int steps = _timestep.Advance(frameSeconds * timeScale);
    StepHitStop(_hitStop, frameSeconds);
    const auto dt = static_cast<float>(_timestep.StepSeconds());
    for (int i = 0; i < steps; ++i) {
        const bool wasPlaying = _session.state == SessionState::Playing;
        Update(dt);
        if (wasPlaying) {
            EmitContinuousParticles(_particles, _session.world, dt);
        }
        FeelEvents();
    }
    FeelEvents();
    // The shake is only a view of the fight, so it moves on with the frames at full speed, which keeps it
    // smooth on a fast monitor and keeps it shaking through a hit stop.
    StepScreenShake(_shake, frameSeconds);
    Draw(static_cast<float>(_timestep.Alpha()));
}

void Game::Update(float dt) {
    // Reading the hardware is the only part of a step that lives in the app. The rules are all in
    // StepSession and StepWorld.
    //
    // The mouse is turned into a world point with the camera as of the last step. The camera drawn on
    // screen is a fraction of a step behind that, which is far below a pixel of aiming error.
    const Camera2D camera = ToRaylibCamera(_session.world.camera, 1.0f);
    const Vec2 aim = AimInGame(CurrentScreenFit());
    const PlayerCommands commands{.move = ReadMoveInput(),
                                  .aimAt = FromRaylib(GetScreenToWorld2D(ToRaylib(aim), camera)),
                                  .fireHeld = ReadFireHeld(),
                                  .firePressed = _pendingPresses.fire,
                                  .selectSlot = _pendingPresses.weaponSlot,
                                  .cycleWeapon = _pendingPresses.cycleWeapon,
                                  .dashPressed = _pendingPresses.dash,
                                  .healPressed = _pendingPresses.heal};
    // Presses are events: only the first step of a frame uses them, so a frame that runs two steps does
    // not fire twice for one click.
    _pendingPresses = {};
    StepSession(_session, commands, dt);
#ifndef NDEBUG
    _lastMoveInput = commands.move;
#endif
}

void Game::FeelEvents() {
    auto& events = _session.world.events;
    for (const WorldEvent& event : events) {
        const ImpactFeel feel = FeelOf(event);
        StartHitStop(_hitStop, feel.slowSeconds);
        AddTrauma(_shake, feel.trauma);
        EmitEventParticles(_particles, event, _session.world);
    }
    events.Clear();
}

void Game::Draw(float alpha) const {
    const ScreenFit fit = CurrentScreenFit();
    BeginDrawing();
    // Black bars where the screen's shape differs from the game's; nothing is drawn on them.
    ClearBackground(BLACK);
    const Vec2 area = kViewSize * fit.scale;
    BeginScissorMode(static_cast<int>(fit.offset.x), static_cast<int>(fit.offset.y), static_cast<int>(area.x + 0.5f),
                     static_cast<int>(area.y + 0.5f));
    BeginMode2D(ScreenCamera(fit));
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, kBackground);
    EndMode2D();

    const World& world = _session.world;
    const bool playing = _session.state == SessionState::Playing;
    // A frozen world has no next step to blend toward. Interpolating it would make everything shake
    // between its last two positions as alpha changes from frame to frame, so it is drawn where it stopped.
    // A world slowed by a hit stop is still moving, so it keeps its interpolation and stays smooth.
    if (!playing) {
        alpha = 1.0f;
    }

    // The world camera, scaled and moved onto the screen with the rest of the game, and shaken. The HUD
    // below is not shaken, so the bars stay readable through the hardest hit.
    BeginMode2D(WorldCamera(world.camera, alpha, fit, CurrentShake(_shake)));
    DrawWorld(world, _particles.particles, alpha, _art);
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugWorld(world.player, world.camera, alpha);
    }
#endif
    EndMode2D();

    // Everything below is in game pixels on the screen and does not move with the camera.
    BeginMode2D(ScreenCamera(fit));
    DrawVignette(_art);
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugHud(world, _lastMoveInput, _particles.particles.Size());
    }
#endif
    DrawHud(world, _session.attempt, playing, AimInGame(fit), _art);
    if (!playing) {
        DrawEndScreen(_session, _art);
    }
    EndMode2D();
    EndScissorMode();
    EndDrawing();
}

} // namespace horde
