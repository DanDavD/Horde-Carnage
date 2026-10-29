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
#include "app/SettingsStore.h"
#include "app/render/ArenaDraw.h"
#include "app/render/BossDraw.h"
#include "app/render/DebugOverlay.h"
#include "app/render/EndScreen.h"
#include "app/render/MenuDraw.h"
#include "app/render/Hud.h"
#include "app/render/Palette.h"
#include "app/render/View.h"
#include "app/render/WorldDraw.h"
#include "core/Facing.h"
#include "core/GamepadAim.h"
#include "core/ImpactFeel.h"
#include "core/MouseAim.h"
#include "core/ScreenFit.h"

namespace horde {

namespace {

// The sounds' own random stream, apart from the fight's and the particles'.
constexpr std::uint64_t kSoundStream = 13;
constexpr float kMenuMoveVolume = 0.35f;
constexpr float kMenuConfirmVolume = 0.5f;

} // namespace

// Every launch starts from a seed the system picks, so no two runs of the game repeat the same fights.
Game::Game(bool windowed)
    : _session{StartSession(kBossArena, kViewSize, std::random_device{}())},
      _particles{SeedParticleEmitter(std::random_device{}())},
      _soundRandom{SeedRandom(std::random_device{}(), kSoundStream)},
      _settings{LoadSettings()} {
    // Vsync caps how often we render, at the monitor's own rate, unless the settings ask for another cap. How
    // often the game simulates comes from FixedTimestep.
#ifdef __EMSCRIPTEN__
    (void)windowed;
    // In a browser the canvas follows the page's size, so going fullscreen on itch.io fills the screen.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
    // Setting a minimum size makes raylib fit the canvas to the page once, right away.
    SetWindowMinSize(kScreenWidth / 4, kScreenHeight / 4);
#else
    // Fullscreen, at the monitor's own resolution: a borderless window over the whole monitor, so switching
    // to another program is instant and the resolution of the desktop is not changed. The settings can ask
    // for a window that can be resized instead, and `--windowed` does for this run only, for recording and
    // debugging.
    if (windowed) {
        _savedFullscreen = _settings.fullscreen;
        _settings.fullscreen = false;
    }
    const unsigned int vsync = _settings.frameRate == FrameRateCap::VSync ? FLAG_VSYNC_HINT : 0;
    SetConfigFlags(vsync | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
    ApplyDisplay();
#endif
    // The default font, which the Debug overlay uses, is drawn scaled with the rest of the picture; smoothed,
    // it scales evenly.
    SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_BILINEAR);
    _art = LoadArt(kBossArena);
    // Every sound is synthesized here, once, in a few hundredths of a second. Without an audio device the
    // game still runs, silent.
    InitAudioDevice();
    _sounds = LoadSoundBank();
    _music = LoadMusicPlayer();
    ApplySettings();
    _backdrop = LoadBackdrop();
    // Esc pauses the fight instead of closing the game; the title's Quit and the window's close button do.
    SetExitKey(KEY_NULL);
    ReturnToTitle(_session);
    ResetHealthTrails();
    _lastMouse = AimInGame(CurrentScreenFit());
}

Game::~Game() {
    UnloadBackdrop(_backdrop);
    UnloadMusicPlayer(_music);
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
    const float frameSeconds = GetFrameTime();
    if (_aimLocked) {
        const Vector2 delta = GetMouseDelta();
        _mouseAim = MoveMouseAim(_mouseAim, Vec2{delta.x, delta.y}, CurrentScreenFit().scale,
                                 MouseAimScale(_settings), kViewSize);
    }
    const Vec2 mouse = MouseAimInGame(CurrentScreenFit());
    const std::optional<int> usedPadSlot = FindActiveGamepadSlot(_gamepadConnection.activeSlot);
    const GamepadConnectionEvents padEvents =
        StepGamepadConnection(_gamepadConnection, ReadGamepadPresence(), usedPadSlot);
    if (padEvents.activeDisconnected) {
        _menuRepeat = {};
        _inputDevice = InputDevice::KeyboardMouse;
    }
    if (_gamepadConnection.activeSlot.has_value()) {
        _gamepadType = ReadGamepadType(_gamepadConnection.activeSlot);
    } else {
        _gamepadType = GamepadType::Generic;
    }
    if (const auto device = DetectActiveInputDevice(mouse, _lastMouse, usedPadSlot)) {
        _inputDevice = *device;
    }
    // Key presses are read once per frame, not per fixed step: a frame with no step would miss them.
    HandleMenus(frameSeconds, padEvents.activeDisconnected);
    _lastMouse = mouse;
    if (_session.state == SessionState::Playing) {
        CollectButtonPresses(_pendingPresses, _gamepadConnection.activeSlot);
    } else {
        // A click or a key in a menu is not a shot or a dash in the fight that follows it.
        _pendingPresses = {};
    }

    // The crosshair replaces the system cursor while playing. On the end screens the cursor is back.
    UpdateAimLock(CurrentScreenFit());
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
    // Cycles on-screen prompts through Keyboard/Mouse, Xbox, PlayStation, Switch, Generic and back to live input.
    if (IsKeyPressed(KEY_F5)) {
        if (!_debugPromptOverride.has_value()) {
            _debugPromptOverride = InputPromptStyle{InputDevice::KeyboardMouse, GamepadType::Generic};
        } else if (_debugPromptOverride->device == InputDevice::KeyboardMouse) {
            _debugPromptOverride = InputPromptStyle{InputDevice::Gamepad, GamepadType::Xbox};
        } else if (_debugPromptOverride->gamepad == GamepadType::Xbox) {
            _debugPromptOverride = InputPromptStyle{InputDevice::Gamepad, GamepadType::PlayStation};
        } else if (_debugPromptOverride->gamepad == GamepadType::PlayStation) {
            _debugPromptOverride = InputPromptStyle{InputDevice::Gamepad, GamepadType::Switch};
        } else if (_debugPromptOverride->gamepad == GamepadType::Switch) {
            _debugPromptOverride = InputPromptStyle{InputDevice::Gamepad, GamepadType::Generic};
        } else {
            _debugPromptOverride.reset();
        }
    }
#endif
    // A hit stop slows the fight's clock, so the world takes fewer of its fixed steps. Every step is still the
    // same length, so the rules play out exactly as at full speed, only slower to watch.
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
    // The music follows the fight in real time: a hit stop slows the fight, not the band.
    UpdateMusicPlayer(_music, _session, frameSeconds);
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

void Game::HandleMenus(float frameSeconds, bool padDisconnected) {
    const SessionState state = _session.state;
    if (state == SessionState::Playing) {
        // The next menu, whichever it is, opens fresh on its first item.
        _menuPage = MenuPage::None;
        _openedPage = MenuPage::None;
        if (ReadPausePressed(_gamepadConnection.activeSlot) || TakePointerLockLost() || padDisconnected) {
            PauseFight(_session);
            OpenPage(MenuPage::Pause, 0);
            _backdrop.ready = false;
            PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
        }
        return;
    }
    // A change of state (a death, a victory, a return to the title) closes whatever was opened over the last
    // page.
    if (PageFor(state) != _menuPage && _openedPage == MenuPage::None) {
        OpenPage(PageFor(state), 0);
    }
    const MenuPage page = CurrentPage();
    if ((state == SessionState::Dead || state == SessionState::Victory) && ReadQuickRetryPressed() &&
        TryRestart(_session)) {
        ForgetLastFight();
        return;
    }
    // The end screens' choices wait for the restart delay, like the restart itself.
    if ((state == SessionState::Dead || state == SessionState::Victory) && !CanRestart(_session)) {
        if (LengthSquared(ReadPadMenuDirection(_gamepadConnection.activeSlot)) == 0.0f) {
            _menuRepeat = {};
        }
        return;
    }

    MenuInput input = ReadMenuInput(_menuRepeat, frameSeconds, _gamepadConnection.activeSlot);
    // In a row of choices, left and right move along it; elsewhere they change the selected value.
    if (IsRowMenu(page)) {
        input.move = input.move != 0 ? input.move : input.adjust;
        input.adjust = 0;
    }
    // Start goes back from the pause menu, as it came in, and from the pages opened over it.
    input.back = input.back ||
                 (state == SessionState::Paused && ReadPausePressed(_gamepadConnection.activeSlot));
    const Vec2 mouse = AimInGame(CurrentScreenFit());
    const std::optional<MenuTarget> under = MenuTargetAt(page, mouse, _art);
    if (mouse != _lastMouse && under) {
        input.hovered = under->item;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && under) {
        input.hovered = under->item;
        if (under->lowerArrow) {
            input.adjust = -1;
        } else {
            input.confirm = true;
        }
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
        Choose(page, result.item);
        break;
    case MenuAction::Adjusted:
        if (page == MenuPage::Settings) {
            const SettingsRow& row = SettingsRows()[static_cast<std::size_t>(result.item)];
            if (row.kind == SettingsRowKind::Value) {
                ChangeSetting(row.setting, result.adjust);
            }
        }
        break;
    case MenuAction::Back:
        GoBack(page);
        break;
    }
}

MenuPage Game::CurrentPage() const {
    return _openedPage != MenuPage::None ? _openedPage : PageFor(_session.state);
}

void Game::OpenPage(MenuPage page, int selected) {
    _openedPage = page == PageFor(_session.state) ? MenuPage::None : page;
    _menu = MakeMenu(MenuItemCount(page));
    _menu.selected = selected;
    _menuPage = page;
    _menuSinceMoved = 0.0f;
    LatchMenuRepeat(_menuRepeat, ReadPadMenuDirection(_gamepadConnection.activeSlot));
}

void Game::Choose(MenuPage page, int item) {
    // A value line changes its value instead, with its own sound.
    if (page != MenuPage::Settings || SettingsRows()[static_cast<std::size_t>(item)].kind != SettingsRowKind::Value) {
        PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
    }
    switch (page) {
    case MenuPage::Title:
        if (item == kTitleBeginItem) {
            BeginFight(_session);
            _pendingPresses = {};
        } else if (item == kTitleSettingsItem) {
            OpenPage(MenuPage::Settings, 0);
        } else {
            _quit = true;
        }
        break;
    case MenuPage::Pause:
        if (item == kPauseResumeItem) {
            ResumeFight(_session);
        } else if (item == kPauseSettingsItem) {
            OpenPage(MenuPage::Settings, 0);
        } else {
            ReturnToTitle(_session);
            ForgetLastFight();
        }
        break;
    case MenuPage::Dead:
    case MenuPage::Victory:
        if (item == 0) {
            TryRestart(_session);
        } else {
            ReturnToTitle(_session);
        }
        ForgetLastFight();
        break;
    case MenuPage::Settings: {
        const SettingsRow& row = SettingsRows()[static_cast<std::size_t>(item)];
        if (row.kind == SettingsRowKind::Value) {
            ChangeSetting(row.setting, 1);
        } else if (row.kind == SettingsRowKind::Controls) {
            OpenPage(MenuPage::Controls, 0);
        } else {
            GoBack(page);
        }
        break;
    }
    case MenuPage::Controls:
        GoBack(page);
        break;
    case MenuPage::None:
        break;
    }
}

void Game::GoBack(MenuPage page) {
    switch (page) {
    case MenuPage::Pause:
        PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
        ResumeFight(_session);
        break;
    case MenuPage::Settings:
        // Back on the item that opened it, as a player expects to find their place.
        PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
        OpenPage(PageFor(_session.state),
                 _session.state == SessionState::Paused ? kPauseSettingsItem : kTitleSettingsItem);
        break;
    case MenuPage::Controls: {
        PlayCue(_sounds, SoundCue{SoundId::MenuConfirm, kMenuConfirmVolume});
        const std::span<const SettingsRow> rows = SettingsRows();
        const auto controls = std::find_if(rows.begin(), rows.end(), [](const SettingsRow& row) {
            return row.kind == SettingsRowKind::Controls;
        });
        OpenPage(MenuPage::Settings, static_cast<int>(controls - rows.begin()));
        break;
    }
    case MenuPage::Title:
    case MenuPage::Dead:
    case MenuPage::Victory:
    case MenuPage::None:
        break;
    }
}

void Game::ChangeSetting(Setting setting, int step) {
    if (!AdjustSetting(_settings, setting, step)) {
        return;
    }
    ApplySettings();
    Settings saved = _settings;
    if (setting == Setting::Display) {
        ApplyDisplay();
        _savedFullscreen.reset();
    } else if (_savedFullscreen) {
        saved.fullscreen = *_savedFullscreen;
    }
    SaveSettings(saved);
    // After the volume has changed, so the tick is heard at the new volume.
    PlayCue(_sounds, SoundCue{SoundId::MenuMove, kMenuMoveVolume});
}

Vec2 Game::MouseAimInGame(const ScreenFit& fit) const {
    return _aimLocked ? _mouseAim : AimInGame(fit);
}

void Game::UpdateAimLock(const ScreenFit& fit) {
    const bool lock = _session.state == SessionState::Playing;
#ifdef __EMSCRIPTEN__
    // The page locks the pointer itself and scales its movement (web/shell.html).
    (void)fit;
    SetCursorHidden(lock);
#else
    if (lock == _aimLocked) {
        return;
    }
    if (lock) {
        // The crosshair starts where the cursor was, and moves on from there by the mouse's movement.
        _mouseAim = AimInGame(fit);
    }
    SetCursorHidden(lock);
    if (!lock) {
        // Unlocked, the cursor comes back where the crosshair was, not in the middle of the screen.
        const Vec2 screen = fit.offset + _mouseAim * fit.scale;
        SetMousePosition(static_cast<int>(screen.x), static_cast<int>(screen.y));
    }
    _aimLocked = lock;
#endif
}

void Game::ApplySettings() {
    SetMasterVolume(MasterGain(_settings));
    SetMusicPlayerVolume(_music, MusicGain(_settings));
    SetMouseSensitivity(MouseAimScale(_settings));
#ifndef __EMSCRIPTEN__
    // In a browser the page draws once per refresh of the screen, whatever is asked.
    if (_settings.frameRate == FrameRateCap::VSync) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
    SetTargetFPS(TargetFramesPerSecond(_settings.frameRate));
#endif
}

void Game::ApplyDisplay() {
#ifndef __EMSCRIPTEN__
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) != _settings.fullscreen) {
        ToggleBorderlessWindowed();
    }
    // raylib keeps its borderless window above every other one, which leaves it covering the desktop after
    // switching to another program; a window the size of the monitor already hides the taskbar.
    if (IsWindowState(FLAG_WINDOW_TOPMOST)) {
        ClearWindowState(FLAG_WINDOW_TOPMOST);
    }
#endif
}

void Game::ForgetLastFight() {
    ClearParticles(_particles);
    ResetHealthTrails();
    _hitStop = HitStop{};
    _shake = ScreenShake{};
    _pendingPresses = {};
    _gamepadAim = GamepadAim{};
}

void Game::Update(float dt) {
    // Reading the hardware is the only part of a step that lives in the app. The rules are all in
    // StepSession and StepWorld.
    //
    // Mouse aim is sent as its screen offset from the view center and resolved inside StepWorld against the
    // camera center of this same step, and gamepad aim is sent as a unit direction, so strafing sideways never
    // bends shots away from the crosshair.
    const Vec2 move = ReadMoveInput(_gamepadConnection.activeSlot);
    const bool fireHeld = ReadFireHeld(_gamepadConnection.activeSlot);

    std::optional<Vec2> aimInView;
    std::optional<Vec2> aimDirection;
    if (_inputDevice == InputDevice::Gamepad) {
        (void)StepGamepadAim(_gamepadAim, _session.world, ReadLookStick(_gamepadConnection.activeSlot),
                             fireHeld || _pendingPresses.fire, StickTurnRadiansPerSecond(_settings) * dt);
        aimDirection = _gamepadAim.direction;
    } else {
        aimInView = MouseAimInGame(CurrentScreenFit()) - kViewSize * 0.5f;
    }

    const PlayerCommands commands{.move = move,
                                  .aimInView = aimInView,
                                  .aimDirection = aimDirection,
                                  .fireHeld = fireHeld,
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
    const InputPromptStyle prompts = CurrentPromptStyle();
    switch (_session.state) {
    case SessionState::Title:
        DrawScene(fit, alpha, false);
        BeginMode2D(ScreenCamera(fit));
        if (_openedPage == MenuPage::None) {
            DrawTitleScreen(_menu, _menuSinceMoved, prompts, _art);
        } else {
            DrawTitleShade();
            DrawOpenedPage();
        }
        EndMode2D();
        break;
    case SessionState::Paused:
        BeginMode2D(ScreenCamera(fit));
        DrawBackdrop(_backdrop, _session.stateSeconds);
        if (_openedPage == MenuPage::None) {
            DrawPauseScreen(_menu, _menuSinceMoved, _session.stateSeconds, prompts, _art);
        } else {
            DrawOpenedPage();
        }
        EndMode2D();
        break;
    case SessionState::Playing:
    case SessionState::Dead:
    case SessionState::Victory:
        DrawScene(fit, alpha, true);
        if (!playing) {
            BeginMode2D(ScreenCamera(fit));
            DrawEndScreen(_session, _menu, _menuSinceMoved, prompts, _art);
            EndMode2D();
        }
        break;
    }
    EndScissorMode();
    EndDrawing();
}

void Game::DrawOpenedPage() const {
    const InputPromptStyle prompts = CurrentPromptStyle();
    DrawPageShade();
    if (_openedPage == MenuPage::Settings) {
        DrawSettingsScreen(_menu, _menuSinceMoved, _settings, prompts, _art);
    } else if (_openedPage == MenuPage::Controls) {
        DrawControlsScreen(_menu, _menuSinceMoved, prompts, _art);
    }
}

InputPromptStyle Game::CurrentPromptStyle() const {
#ifndef NDEBUG
    if (_debugPromptOverride.has_value()) {
        return *_debugPromptOverride;
    }
#endif
    return InputPromptStyle{.device = _inputDevice, .gamepad = _gamepadType};
}

void Game::DrawScene(const ScreenFit& fit, float alpha, bool withHud) const {
    BeginMode2D(ScreenCamera(fit));
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, kBackground);
    EndMode2D();

    const World& world = _session.world;
    const bool playing = _session.state == SessionState::Playing;
    const InputPromptStyle prompts = CurrentPromptStyle();
    const Vec2 playerPos = Lerp(world.player.motion.previousPosition, world.player.motion.position, alpha);

    Vec2 screenAim{};
    std::optional<Vec2> liveFacing;
    if (withHud) {
        if (prompts.device == InputDevice::Gamepad) {
            const Camera2D camera = ToRaylibCamera(world.camera, alpha);
            const Vec2 worldAim = GamepadAimPoint(playerPos, {}, _gamepadAim.direction, kGamepadCrosshairDistance);
            const Vector2 screenVec = GetWorldToScreen2D(ToRaylib(worldAim), camera);
            screenAim = Vec2{screenVec.x, screenVec.y};
            if (playing) {
                liveFacing = _gamepadAim.direction;
            }
        } else {
            screenAim = MouseAimInGame(fit);
            if (playing) {
                const Vec2 viewCenter = Lerp(world.camera.previousCenter, world.camera.center, alpha);
                const Vec2 worldAim = viewCenter + (screenAim - kViewSize * 0.5f);
                Vec2 facing = world.player.facing;
                AimFacing(facing, playerPos, worldAim);
                liveFacing = facing;
            }
        }
    }

    // The world camera, scaled and moved onto the screen with the rest of the game, and shaken. The HUD
    // below is not shaken, so the bars stay readable through the hardest hit.
    ShakeOffset shake = CurrentShake(_shake);
    const float shakeAmount = ShakeAmount(_settings.shake);
    shake.offset = shake.offset * shakeAmount;
    shake.turnDegrees *= shakeAmount;
    BeginMode2D(WorldCamera(world.camera, alpha, fit, shake));
    // The boss's death plays out over the victory; a boss that is not dead ignores it.
    const float bossSinceDied = _session.state == SessionState::Victory ? _session.stateSeconds : kBossDeathSeconds;
    DrawWorld(world, _particles.particles, alpha, _art, bossSinceDied, playing, liveFacing);
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
        DrawDebugHud(world, _lastMoveInput, _particles.particles.Size(), _gamepadConnection.activeSlot,
                     _gamepadType, ReadGamepadName(_gamepadConnection.activeSlot), _music);
    }
#endif
    if (withHud) {
        DrawHud(world, _trails, _session.attempt, playing, screenAim, prompts, _art);
    }
    EndMode2D();
}

} // namespace horde
