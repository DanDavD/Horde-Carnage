#include "app/Game.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <span>

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"
#include "app/render/ArenaDraw.h"
#include "app/render/BossDraw.h"
#include "app/render/DebugOverlay.h"
#include "app/render/EndScreen.h"
#include "app/render/MenuDraw.h"
#include "app/render/Hud.h"
#include "app/render/Palette.h"
#include "app/render/View.h"
#include "app/render/WorldDraw.h"
#include "core/ImpactFeel.h"
#include "core/ScreenFit.h"

namespace horde {

namespace {

// The sounds' own random stream, apart from the fight's and the particles'.
constexpr std::uint64_t kSoundStream = 13;
// Under full volume, so the fight's loudest moments, mixed together, do not clip.
constexpr float kMasterVolume = 0.8f;
constexpr float kMenuMoveVolume = 0.35f;
constexpr float kMenuConfirmVolume = 0.5f;

} // namespace

// Every launch starts from a seed the system picks, so no two runs of the game repeat the same fights.
Game::Game(bool windowed)
    : _session{StartSession(kBossArena, kViewSize, std::random_device{}())},
      _particles{SeedParticleEmitter(std::random_device{}())},
      _soundRandom{SeedRandom(std::random_device{}(), kSoundStream)} {
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
    // Every sound is synthesized here, once, in a few hundredths of a second. Without an audio device the
    // game still runs, silent.
    InitAudioDevice();
    SetMasterVolume(kMasterVolume);
    _sounds = LoadSoundBank();
    _backdrop = LoadBackdrop();
    // Esc pauses the fight instead of closing the game; the title's Quit and the window's close button do.
    SetExitKey(KEY_NULL);
    ReturnToTitle(_session);
    ResetHealthTrails();
}

Game::~Game() {
    UnloadBackdrop(_backdrop);
    UnloadSoundBank(_sounds);
    CloseAudioDevice();
    UnloadArt(_art);
    CloseWindow();
}

void Game::Run() {
#ifdef __EMSCRIPTEN__
    // A browser tab cannot be held in a loop: the page calls one frame at a time, at its own refresh rate.
    // With the last argument set, this never returns, so `this` stays alive for the life of the page.
    emscripten_set_main_loop_arg([](void* game) { static_cast<Game*>(game)->Frame(); }, this, 0, true);
#else
    while (!WindowShouldClose() && !_quit) {
        Frame();
    }
#endif
}

void Game::Frame() {
    // Key presses are read once per frame, not per fixed step: a frame with no step would miss them.
    HandleMenus();
    if (_session.state == SessionState::Playing) {
        CollectButtonPresses(_pendingPresses);
    } else {
        // A click or a key in a menu is not a shot or a dash in the fight that follows it.
        _pendingPresses = {};
    }

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
    _menuSinceMoved += frameSeconds;
    // Behind the title and the pause menu everything holds still: the particles, the slow-down and the
    // shake pick up where they were when the fight resumes.
    const bool held = _session.state == SessionState::Title || _session.state == SessionState::Paused;
    const float timeScale = held ? 0.0f : TimeScale(_hitStop);
    // Existing particles advance first, scaled by the hit stop so shards and casings hang in slow motion with
    // the fight, while any burst spawned in this frame's steps is drawn once right where it started.
    StepParticles(_particles.particles, _session.world.arena, std::min(frameSeconds, 0.25f) * timeScale);
    const int steps = _timestep.Advance(frameSeconds * (held ? 1.0f : timeScale));
    if (!held) {
        StepHitStop(_hitStop, frameSeconds);
    }
    const auto dt = static_cast<float>(_timestep.StepSeconds());
    for (int i = 0; i < steps; ++i) {
        const bool wasPlaying = _session.state == SessionState::Playing;
        Update(dt);
        StepHealthTrails(dt);
        if (wasPlaying) {
            EmitContinuousParticles(_particles, _session.world, dt);
        }
        FeelEvents();
    }
    FeelEvents();
    // The shake is only a view of the fight, so it moves on with the frames at full speed, which keeps it
    // smooth on a fast monitor and keeps it shaking through a hit stop.
    if (!held) {
        StepScreenShake(_shake, frameSeconds);
    }
    // The fight behind the pause menu is drawn once, into the blurred still, when the pause begins.
    if (_session.state == SessionState::Paused && !_backdrop.ready) {
        BeginBackdropCapture(_backdrop);
        DrawScene(ScreenFit{}, 1.0f, true);
        EndBackdropCapture(_backdrop);
    }
    Draw(static_cast<float>(_timestep.Alpha()));
}

void Game::HandleMenus() {
    const SessionState state = _session.state;
    if (state == SessionState::Playing) {
        // The next menu, whichever it is, opens fresh on its first item.
        _menuState = SessionState::Playing;
        if (ReadPausePressed() || TakePointerLockLost()) {
            PauseFight(_session);
            _backdrop.ready = false;
            PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
        }
        return;
    }
    const std::span<const char* const> labels = MenuLabelsFor(state);
    if (state != _menuState) {
        _menu = MakeMenu(static_cast<int>(labels.size()));
        _menuState = state;
        _menuSinceMoved = 0.0f;
    }
    if ((state == SessionState::Dead || state == SessionState::Victory) && ReadQuickRetryPressed() &&
        TryRestart(_session)) {
        ForgetLastFight();
        return;
    }

    MenuInput input = ReadMenuKeys();
    // Start goes back from the pause menu, as it came in.
    input.back = input.back || (state == SessionState::Paused && ReadPausePressed());
    const MenuLayout layout = MenuLayoutFor(state);
    const Vec2 mouse = AimInGame(CurrentScreenFit());
    const std::optional<int> under = MenuItemAt(layout, labels, mouse, _art);
    if (mouse != _lastMouse) {
        input.hovered = under;
    }
    _lastMouse = mouse;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && under) {
        input.hovered = under;
        input.confirm = true;
    }
    // The end screens' choices wait for the restart delay, like the restart itself.
    if ((state == SessionState::Dead || state == SessionState::Victory) && !CanRestart(_session)) {
        return;
    }

    const MenuResult result = StepMenu(_menu, input);
    switch (result.action) {
    case MenuAction::None:
        break;
    case MenuAction::Moved:
        _menuSinceMoved = 0.0f;
        PlayCue(_sounds, SoundCue{SoundId::MenuMove, kMenuMoveVolume});
        break;
    case MenuAction::Chosen:
        PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
        Choose(result.item);
        break;
    case MenuAction::Back:
        if (state == SessionState::Paused) {
            PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
            ResumeFight(_session);
        }
        break;
    }
}

void Game::Choose(int item) {
    switch (_session.state) {
    case SessionState::Title:
        if (item == 0) {
            BeginFight(_session);
            _pendingPresses = {};
        } else {
            _quit = true;
        }
        break;
    case SessionState::Paused:
        if (item == 0) {
            ResumeFight(_session);
        } else {
            ReturnToTitle(_session);
            ForgetLastFight();
        }
        break;
    case SessionState::Dead:
    case SessionState::Victory:
        if (item == 0) {
            TryRestart(_session);
        } else {
            ReturnToTitle(_session);
        }
        ForgetLastFight();
        break;
    case SessionState::Playing:
        break;
    }
}

void Game::ForgetLastFight() {
    ClearParticles(_particles);
    ResetHealthTrails();
    _hitStop = HitStop{};
    _shake = ScreenShake{};
    _pendingPresses = {};
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

void Game::StepHealthTrails(float dt) {
    const World& world = _session.world;
    StepTrailingBar(_trails.player, world.player.health.current, world.player.health.max, dt);
    StepTrailingBar(_trails.boss, world.boss.health.current, world.boss.health.max, dt);
}

void Game::ResetHealthTrails() {
    const World& world = _session.world;
    _trails = HealthTrails{.player = TrailingBarAt(world.player.health.current),
                           .boss = TrailingBarAt(world.boss.health.current)};
}

void Game::FeelEvents() {
    auto& events = _session.world.events;
    for (const WorldEvent& event : events) {
        const ImpactFeel feel = FeelOf(event);
        StartHitStop(_hitStop, feel.slowSeconds);
        AddTrauma(_shake, feel.trauma);
        EmitEventParticles(_particles, event, _session.world);
        if (const std::optional<SoundCue> cue = CueFor(event, _soundRandom)) {
            const FollowCamera& camera = _session.world.camera;
            PlayCue(_sounds, Placed(*cue, event.position, camera.center, camera.viewSize.x));
        }
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
    const bool playing = _session.state == SessionState::Playing;
    // A frozen world has no next step to blend toward. Interpolating it would make everything shake
    // between its last two positions as alpha changes from frame to frame, so it is drawn where it stopped.
    // A world slowed by a hit stop is still moving, so it keeps its interpolation and stays smooth.
    if (!playing) {
        alpha = 1.0f;
    }
    switch (_session.state) {
    case SessionState::Title:
        DrawScene(fit, alpha, false);
        BeginMode2D(ScreenCamera(fit));
        DrawTitleScreen(_menu, _menuSinceMoved, _art);
        EndMode2D();
        break;
    case SessionState::Paused:
        BeginMode2D(ScreenCamera(fit));
        DrawBackdrop(_backdrop, _session.stateSeconds);
        DrawPauseScreen(_menu, _menuSinceMoved, _session.stateSeconds, _art);
        EndMode2D();
        break;
    case SessionState::Playing:
    case SessionState::Dead:
    case SessionState::Victory:
        DrawScene(fit, alpha, true);
        if (!playing) {
            BeginMode2D(ScreenCamera(fit));
            DrawEndScreen(_session, _menu, _menuSinceMoved, _art);
            EndMode2D();
        }
        break;
    }
    EndScissorMode();
    EndDrawing();
}

void Game::DrawScene(const ScreenFit& fit, float alpha, bool withHud) const {
    BeginMode2D(ScreenCamera(fit));
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, kBackground);
    EndMode2D();

    const World& world = _session.world;
    const bool playing = _session.state == SessionState::Playing;

    // The world camera, scaled and moved onto the screen with the rest of the game, and shaken. The HUD
    // below is not shaken, so the bars stay readable through the hardest hit.
    BeginMode2D(WorldCamera(world.camera, alpha, fit, CurrentShake(_shake)));
    // The boss's death plays out over the victory; a boss that is not dead ignores it.
    const float bossSinceDied = _session.state == SessionState::Victory ? _session.stateSeconds : kBossDeathSeconds;
    DrawWorld(world, _particles.particles, alpha, _art, bossSinceDied);
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugWorld(world.player, world.camera, alpha);
    }
#endif
    EndMode2D();

    // Everything below is in game pixels on the screen and does not move with the camera.
    BeginMode2D(ScreenCamera(fit));
    DrawVignette(_art);
    DrawPainVignette(_art, world.player.health, world.elapsedSeconds);
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugHud(world, _lastMoveInput, _particles.particles.Size());
    }
#endif
    if (withHud) {
        DrawHud(world, _trails, _session.attempt, playing, AimInGame(fit), _art);
    }
    EndMode2D();
}

} // namespace horde
