# Architecture

No game engine. An SFML window and a variable-timestep loop drive a stack of
states; the gameplay state runs a hand-written, headless simulation that
input events feed into. Roughly 25,000 lines of C++23 across ~180 files.

For how the common game-programming patterns map onto this code, see
[PATTERNS.md](PATTERNS.md).

---

## The loop

`main()` pins one thing down before anything else: a compile-time
`static_assert` on `SFML_VERSION_MAJOR/MINOR/PATCH` (== 3.1.0), so a mismatched
SFML build fails at compile time instead of misbehaving at runtime.

`Application`'s constructor then:

1. loads settings (`SettingsManager::Load`) and the piece / SRS kick JSON
   overrides
2. opens the window and applies display settings, falling back to the
   desktop resolution on a first run
3. loads every texture and shader **synchronously**, on the main thread —
   these are GPU objects, and creating them off-thread risks a driver
   deadlock on some vendors
4. loads the loading-screen font and just enough of the main-menu music
   stream to start it immediately
5. loads the localization catalog and high scores
6. pushes `LoadingState`, which streams every remaining heavyweight asset on
   a background thread while the main thread renders an animated
   block-by-block progress bar

`Run()` is the classic shape:

```cpp
sf::Clock clock;
while (window.isOpen())
{
    const float frameSeconds = clock.restart().asSeconds();
    const float deltaTime = std::min(frameSeconds, MaxFrameTime);   // clamp

    HandleInput();       // poll events -> gamepad / cursor / window / current state
    // apply any pending state-machine change
    // apply any pending display-mode change
    Update(deltaTime);
    // apply pending changes again
    Render();

    audioPlayer.RemoveStoppedSounds();
}
```

Every piece of motion is multiplied by `deltaTime`, so gravity and animation
speed are frame-rate independent. `std::min(frameSeconds, MaxFrameTime)` is
the safety valve: if the game hangs for a second (alt-tab, a Windows dialog),
an unclamped `deltaTime` of 1.0 would drop a falling piece a full second's
worth of rows in one step. The clamp caps the simulated step so a stall
slows the game down instead of skipping state.

`Render()` draws into an off-screen `sf::RenderTexture` at a fixed virtual
1920×1080, then draws that texture to the actual window through the CRT
fragment shader (when enabled) — so every screen is authored and laid out at
one resolution regardless of the window's real size. The FPS counter is
drawn last, directly to the window, so it stays crisp and un-shaded.

---

## Layers

| Layer | Folder | Responsibility |
|---|---|---|
| App | `src/app` | window, main loop, boot sequence, top-level wiring |
| Core | `src/core` | `Context` (shared service locator), `State` / `StateMachine`, `GameVersion` |
| States | `src/states` | every screen and transition, on the `StateMachine` |
| UI | `src/ui` | the reusable widget/animation layer every screen is built from |
| Gameplay | `src/gameplay` | `Board`, `Tetromino`, `TetrominoBag`, SRS kick/piece data, `TSpinRule`, `GameplaySession`, `EscalationDirector` — pure rules, no rendering |
| Rendering | `src/rendering` | `BoardRenderer`, `BoardCallouts`, `EffectsController`, `GameplayHUD`, `SceneMotion` |
| Input | `src/input` | action map, key rebinding, `input/gamepad/` (layout detection, haptics, prompts) |
| Audio | `src/audio` | `AudioPlayer`, `MusicPlayer`, `AudioBalance` |
| Settings | `src/settings` | `GameSettings`, `ControlSettings`, `SettingsManager` |
| Localization | `src/localization` | `Language`, `LocalizationManager`, `TextKeys` |
| Statistics | `src/statistics` | `HighScoreEntry`, `HighScoreManager` |
| Display | `src/display` | `DisplayManager`, `DisplaySettings` |
| Haptics | `src/haptics` | `HapticSettings` — design-time rumble / lightbar tuning data, JSON-backed |
| Resources | `src/resources` | `Assets` (IDs, paths), `ResourceManager` (typed cache) |
| Primitives | `src/primitives` | `NeonGlow`, `NineSliceFrame`, `PixelDust` |
| Loading | `src/loading` | `AssetLoadJob`, `LoadingProgress` — the background streaming behind `LoadingState` |
| Utils | `src/utils` | `AppDataPath`, `SafeFileWrite`, `Easing`, `Random`, `TimeFormat` |

Namespaces mirror the folders where it helps: `UI::` for `src/ui`,
`Haptics::` for the gamepad haptics helpers, `AppDataPath::` /
`SafeFileWrite::` for the two persistence utilities.

---

## Gameplay: rules vs. reaction

`GameplaySession` is the whole rules engine, and it knows nothing about SFML:
no rendering, no audio, no input polling. It exposes a small surface (move,
rotate, soft/hard drop, hold) and, once per `Update()`, a batch of `Events` —
what happened this frame (landed, detected rows, cleared rows, leveled up,
game over, a T-spin, a Speed Surge starting, a garbage row pushed...).

`GameplayInputController` turns keyboard/gamepad input into calls on the
session, plus the feedback tied directly to that input (a move's footstep, a
rotate's click, a wall bump's shake) — sound and haptics that belong to the
action itself, not to what the simulation decided happened.

`GameplayState::ReactToEvents` is the other half: it drains
`session.ConsumeEvents()` and turns *simulation* outcomes into everything
else — row-clear callouts, the correct haptic pulse for a Tetris vs. a
Perfect Clear, HUD updates, the death beat that leads into `GameOverState`.
Splitting it this way means an input-driven effect (wall bump) and an
event-driven one (Tetris flash) never have to share a branch that's really
answering two different questions: "what did the player just do" and "what
did that cause."

`EscalationDirector` sits beside the session as the endless mode's difficulty
clock — see [GAMEPLAY.md#escalation](GAMEPLAY.md#escalation) for the tiers it
drives.

---

## Data-driven content

Piece shapes (`assets/data/pieces.json`) and SRS wall-kick tables
(`assets/data/srs_kicks.json`) load through `PieceDataFile` / `KickDataFile`
at startup, with a hardcoded fallback if the file is missing or malformed —
so a bad JSON edit degrades to "standard rules" instead of crashing. Haptic
tuning (`HapticSettings`) and per-track audio balance (`AudioBalance`) follow
the same pattern. Everything else — the level/gravity curve, scoring table,
escalation timings — is small enough to stay in code as named constants
rather than external data, per the project's own "data-driven, selectively"
rule: author content that changes often or that a designer might want to
retune without a rebuild; keep the core loop in code.

---

## Persistence

Two files live under `%LOCALAPPDATA%\Alone Bull Company\Tessera\`
(`AppDataPath::Resolve`, falling back to a `user_data\` folder beside the
executable if `LOCALAPPDATA` can't be read): `settings.json` and
`scores.json`. Both go through `SafeFileWrite` — write to a temp file, then
atomically replace the real one, so a crash or a full disk mid-write can't
leave a half-written file behind. A file that exists but fails to parse
(corruption, or a save from an incompatible `FormatVersion`) is renamed to
`<name>.corrupt` rather than deleted or silently overwritten, and the game
falls back to defaults for that file instead of refusing to start.

---

## Localization

`LocalizationManager` loads one JSON catalog per `Language` and merges it
over the English catalog, so a partially translated language still falls
back to English for any missing key rather than showing a blank string or a
key name. `TextKeys` are compile-time string constants naming catalog
entries (`TextKey::Options::Gameplay`, ...) — a typo there is a compile
error, not a silent lookup miss at runtime.
