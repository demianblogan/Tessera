# Tessera — Development Backlog

Source of truth for completed work, the version in progress, and planned
versions. Update it whenever an item changes scope or release.

Two phases, following the *Until Last Asteroid* (ULA) process:

1. **Phase 1 — code maturity (`v1.0.x`).** No new gameplay. Turn the
   prototype-grade code into a professional, bug-free, de-duplicated, well
   layered codebase, porting proven systems from ULA.
2. **Phase 2 — features (`v1.1.0`+).** New systems and content on the
   cleaned-up foundation.

Each version: its own `v1.0.x-<slug>` branch (no prefix), a PR titled
`v<version> — <Name>`, a `v<version>` tag, and a GitHub Release. Commit style
follows ULA: imperative subject + a "why" body.

Cross-cutting decisions:

- **Name:** Tessera (Latin for a mosaic tile).
- **UI:** a reusable, cross-project UI library built on the retained
  widget-tree + `Measure`/`Arrange` layout pass, completed with an input/focus
  layer and ULA's reusable rendering helpers. Intended to be shared back into
  ULA and project 3.
- **No ECS** — one dynamic piece + a 10×20 grid does not need it.
- **Data-driven, selectively** — authored content (piece defs, SRS kicks,
  scoring, gravity curve, mode params, challenge stages, achievements,
  localization) as JSON under `assets/data/`. Runtime/user data goes to
  `%LOCALAPPDATA%`, never the repo. Core loop stays code.
- **Dead-code removal** — one dedicated project-wide pass at the end of Phase 1,
  not piecemeal. Ports are done whole, not pre-trimmed.

---

## Released

### v1.0

- Original portfolio version: playable Tetris with 7-bag pieces, ghost piece,
  progressive difficulty, next-piece preview, screen shake, row-clear and
  landing effects, a custom UI framework, a settings menu, top-5 high scores,
  and shader-based CRT / blur / glow post-processing.

---

## Released

### v1.2.0 — Menu shell & Options

Originally scoped as "Modern rules"; **re-scoped** to the menu shell and a
complete Options screen (the SRS / guideline-rules work moved to v1.3.0). No
gameplay-rules changes.

- **Menu shell** (`states/MenuShell` hosting `states/MenuScreen`s;
  `MainMenuScreen` / `CreditsScreen` / `OptionsScreen`). Activating a ring
  entry: the title flies up, the entries disintegrate to pixels, and the chosen
  entry morphs into the sub-screen header (`ui/MenuHeader` `RiseFrom` /
  `SinkTo`); reversed on Back. The shell owns the shared ambient
  (aurora / backdrop / sparks / version / lightbar). Back-out returns focus to
  the entry you left from.
- **Credits screen** — a 9-slice framed panel (`ui/NineSliceFrame`,
  `assets/textures/ui/frame.png`): developer blurb, email, YouTube, source
  link; a main-menu-style "Back to Main Menu" text button. Gold accent.
- **Options screen** (`states/OptionsScreen`) — a left category column
  (`ui/MenuButtonColumn`) with a right-hand panel that previews on hover and
  opens in place, the column compacting around the open entry.
  - **Gameplay** (teal) — Gamepad Vibration / Gamepad Lightbar / Screen Shake
    toggles.
  - **Graphics** (sky blue) — Screen Resolution + Window Mode carousels,
    Vertical Sync / Show FPS / CRT Filter toggles; a `display/DisplayManager`
    with deferred window recreation and letterboxing; Borderless locks the
    resolution. FPS-limit and block-style settings cut from the game.
  - **Audio** (green) — Sound / Music volume sliders (10 % steps).
  - **Controls** (violet) — its own slide-in sub-page: **Keyboard** (rebind the
    six gameplay keys; blink-to-capture, Escape / gamepad-B cancels, reserved
    and clashing keys rejected with a flash; layout-independent key names via
    `input/KeyName`) and **Gamepad** (read-only Xbox / PlayStation reference
    table from button-prompt atlases).
  - **Language** (rose) — slide-in sub-list, English live, the other four greyed
    (UI scaffold; localisation itself deferred).
  - Every panel: an Apply / Reset / Back row with dirty / at-defaults tracking
    and an unsaved-changes dialog (`states/SettingsCategoryPanel` base,
    `ui/ConfirmDialog`, `ui/OptionRow` + carousel / slider / toggle / key-bind
    rows).
- **Settings persistence** — `GameSettings` format 5: display, the graphics
  toggles, volumes, the six rebindable scancodes, the three gameplay toggles.
- **`haptics.json` lightbar map** — any `[r,g,b]` key; menus resolve their
  focused item through `HapticSettings::LightbarFor` so the DualSense resting
  colour is hand-calibratable. Menu navigation no longer flashes it; each
  TESSERA title letter snaps it to its own colour on landing.
- Gamepad rotation moved from the analog triggers to the shoulder buttons;
  gamepad detection re-scans each frame while none is connected.
- Quit-out-of-game removed from the main menu (only the "Quit" ring entry).

### v1.1.0 — Presentation

The first feature version: a proper opening sequence, a rebuilt main menu, the
gamepad-haptics port, and a data-driven audio mix. No gameplay-rules changes.

- **Loading screen** (`states/LoadingState`, `loading/AssetLoadJob` +
  `LoadingProgress`) — a tetromino-block progress bar; textures and shaders
  load synchronously (GPU work on a background thread deadlocks some drivers),
  a `std::jthread` streams the audio and fonts. Hands off to:
- **Company splash** (`states/CompanySplashState`) — fade in / hold / fade out
  over the logo; any input skips.
- One **shell music track** plays unbroken across loading → splash → menu.
- **Main menu, rebuilt** (`states/MainMenuState`, no longer `MenuScreenState`):
  - `ui/DropInTitle` — the "TESSERA" letters fall in one by one, squash and
    spring, then breathe; per-letter tetromino colour, dark outline, gradient
    fill, neon bloom, impact flash / shockwave / chromatic split, a landing
    sound with rising pitch and a growing gamepad pulse.
  - `ui/CarouselMenu` — entries on a shallow 3-D ring: front large and lit,
    sides dim and soft, back tucked behind the title. Left / right rotates it
    (keyboard, gamepad, or on-screen arrows with press feedback); it flies in
    like cars merging onto a roundabout, each with a swoosh and a rumble. Front
    entry glows in its colour; arrival flash on lock. Achievements and Credits
    are in as disabled placeholders.
  - Ambient background: `ui/MenuBackdrop` (drifting tetrominoes, shoved by
    navigation), `ui/MenuSparks` (rising embers), `ui/MenuAurora`
    (`menu_aurora.frag`).
  - The DualSense lightbar tracks the selected entry's colour.
- **`ui/GlowingCursor`** — the game draws its own cursor (pulsing glow, no halo,
  no trail), part of the scene so it gets the CRT pass; the OS cursor is hidden
  and ours shows only while the mouse is the active device.
- **`ui/FpsCounter`** + a "Show FPS" toggle in Graphics; grey version text
  bottom-right (`core/GameVersion.h`).
- **Gamepad haptics** (`input/gamepad/GamepadHaptics` + `HapticProfiles`,
  ported from ULA; `libs/DualSenseWindows/` vendored as source): rumble on
  land / hard-drop / wall / row-clear / tetris / level-up / game-over; a
  by-state lightbar; pitch-varied navigation ticks. Adaptive-trigger code is
  carried but dormant.
- **`audio/AudioBalance`** — per-sound / per-track volume coefficients from
  `assets/data/audio_balance.json` (`libs/nlohmann/json.hpp` vendored), tunable
  without a rebuild. `AudioPlayer` reworked to heap-own its voices and reclaim
  them by wall-clock (SFML 3's status poll cut short sounds).
- **CRT shader** keeps its scanlines / aberration / flicker / vignette, loses
  the screen curvature and edge cropping.
- `assets/music/` + `assets/sounds/` merged under `assets/audio/`.

### v1.0.9 — Polish pass

The end-of-Phase-1 cleanup, widened after a full read-through (a review put the
code at "solid Middle, near Middle-Senior"). No behaviour changes.

- **`MenuScreenState`** — MainMenu / Pause / GameOver shared a copy each of the
  button factory, `MenuInput` dispatch, mouse forwarding, layout refresh and the
  MenuList callback wiring. That's the base now; a subclass builds its layout,
  calls `AddMenuItem(text, callback)`, and overrides `OnBack()` /
  `HandleExtraEvent()`. The per-screen action enum and `PerformAction` switch
  are gone.
- **Pause backdrop** — `Application::Render` hard-coded `StateId::Pause` to blur
  the layer below, while the generic path (`IsTransparent()` +
  `StateMachine::RenderStates`' transparent-walk) ran only where nothing is ever
  transparent. Replaced with `State::GetBackdrop()`; the whole `StateId` enum
  and `GetId()` (its only consumer) are removed, along with the dead walk.
- **`SettingsRowList`** — the Settings screen's tagged-union selection model
  (`SelectableElement`, `selectedIndex`, the three-way highlight chain, a method
  per direction) is one focused class now: rows, current index, highlight,
  hit-testing, click-drag, and `onButton/Slider/Selection` callbacks.
- **`Layout::Arrange` / `Measure`** — the vertical and horizontal branches were
  mirror images and the size-rule switch was written three times; an `Axes`
  helper collapses each to one path. Fixes the latent cross-axis Fill case and
  the missing `Alignment::Start` in the horizontal branch.
- **Dead code** — `Layout::GetChildren` / `SetBackgroundColor` (+ member +
  render branch), `Label::GetVisualHeight`, `ActionMap::ClearBindings`,
  `ResourceManager::ForEach() const`, `GameplayState::nextTetrominoLabel`, two
  stray `SettingsState` constants.
- Minor: `Application` ctor initialiser list follows declaration order;
  `Board::Contains` / `IntersectsLockedCells` are private (`CanPlace` is the
  only caller).

`UI::MenuList` and `SettingsRowList` remain two selection models — merging them
is left to the Phase 2 main-menu overhaul, which rewrites navigation anyway.

Also landed here, ahead of when it's used: **`input/gamepad/GamepadHaptics`**,
ported whole from ULA (self-contained, no SFML). Xbox rumble via XInput,
DualSense rumble + lightbar + adaptive triggers via the vendored
`libs/DualSenseWindows` (MIT, raw HID, built as source at `/W3`). Wired into
`Application` (owned, ticked) and `GamepadManager::SetHaptics` (a faint tick on
every gamepad menu move). The gameplay pulses, the by-state lightbar, the
trigger effects on rotate, and the vibration settings are v1.1.0.

### v1.0.8 — Localization pipe

Every on-screen string moved from its call site into a flat catalog
(`assets/data/localization/en.txt`, `section.key = value`, `#` comments, `\n`,
UTF-8). `LocalizationManager::GetText` / `FormatText` (with `{token}`
placeholders); `localization/TextKeys.h` holds every key as an
`inline constexpr string_view` grouped by screen. `Context` carries a
`LocalizationManager&`; all six states and the HUD pull their text from it. A
missing key renders as `<key>` rather than crashing. A `Language` enum,
per-language fonts and a live-switch revision counter are left for the v1.4.0
language work. English only; no JSON dependency.

### v1.0.7 — Visual pass & first tests

Foundations for the look, and a safety net under the rules. No gameplay changes.

- **`rendering/NeonGlow`** — real additive bloom for the active piece and the
  selected menu button: source rendered to an off-screen buffer, silhouette
  grown by a square dilation (`neon_dilate.frag`) so the edge-glow is even on
  every side and corner, softened by a separable Gaussian (`neon_blur.frag`),
  composited back tinted and pulsing. Replaces the fake 1.18×-scale glow and the
  flat `glow.frag` menu highlight (both removed). `crt.frag` now uses its `time`
  uniform, silencing the `Uniform "time" not found` launch warning.
- **`ui/NineSliceFrame`** in `Button` / `Panel` — corners stay unscaled, edges
  and centre stretch. *Note:* the current `panel_background` / `button_background`
  art isn't symmetric, so the payoff is small until those are replaced (see
  Phase 2 visual overhaul).
- **`ui/TextLayout`** — `FitWidth` scales text down to a max width instead of
  changing character size (no per-string atlas rebuild); `CentreOrigin`.
  `Label::SetMaxWidth`; the gameplay HUD caps its labels so the controls block
  no longer overruns its panel.
- **`/W4` → 0 + warnings-as-errors.** C4244 fixed, C4100 fixed, C4458 disabled
  project-wide (the deliberate ctor/param-name idiom).
- **First unit tests** — a `TesseraTests` project (own `.vcxproj`, in the
  solution, out of the game build) on doctest: 25 cases over `Board`,
  `Tetromino`, `TetrominoBag`, `GameplaySession`.

`ScreenFade` and `GlowingCursor` were dropped — cursor design and menu
transitions are a v1.1 discussion.

### v1.0.6 — GameState split

Broke the ~950-line `GameState` god-class into model / state / renderer /
effects. Behaviour unchanged — relocation only.

- **`src/game/` → `src/gameplay/`** (matches ULA's own `game` → `gameplay`
  rename, done for the same "what lives where?" reason).
- **`gameplay/GameplaySession`** — the pure rules and state of one playthrough:
  board, bag, active + next piece, gravity, row-clear delay, scoring, levelling.
  No SFML, no `Context`. The host pushes input intents, calls `Update(dt)`, then
  drains `ConsumeEvents()` (landed / rows detected / rows cleared / levelled up /
  game over). Named for the concept, not the game, so a future rename of Tessera
  touches nothing here. (ULA calls the same thing `GameplaySession` too.)
- **`states/GameplayState`** (was `GameState`) — owns the session, the input
  layer, the HUD, and the renderers; turns session events into sound, HUD text
  and screen effects.
- **`rendering/BoardRenderer`** — board gradient, walls, locked cells, ghost,
  active piece (glow + normal), next-piece preview. Reads the session only.
- **`rendering/EffectsController`** — the timer state machine behind screen
  shake, the landing flash and the row-clear flash / sweep.
- Restored a lost UTF-8 BOM that had turned the controls-panel arrows to mojibake.

### v1.0.5 — Input abstraction, gamepad & DAS/ARR

- **`input/InputBinding` + `ActionMap<Action>` + `InputHandler<Action>`**
  (ported from ULA): a physical input (`sf::Keyboard::Scancode` or mouse button)
  + a trigger (`OnPress` / `OnRelease` / `WhileHeld`), a per-action bindings
  table, and a callback dispatcher. Gamepad is **not** in the binding variant
  (a d-pad is a POV axis, button indices are vendor-specific) — same reason ULA
  keeps `GamepadManager` separate.
- **`input/GamepadManager`** (ported, slimmed): Xbox / PlayStation by USB
  vendor id, stick dead zones, menu `NavigationAction`, pause, and polled
  gameplay queries. The twin-stick aim code is dropped.
- **`input/DirectionalRepeater`** (new): DAS/ARR for left/right — one step on
  press, a pause, then auto-repeat. The main "feel" fix; ULA has no equivalent.
- **`input/MenuInput::Resolve`**: one keyboard-or-gamepad event → one
  `MenuInput::Action`. MainMenu / Pause / GameOver / Settings / Statistics now
  handle that enum instead of raw scancodes, so the gamepad drives every menu
  for free.
- **Mouse in menus**: `MenuList::PointerPressed`; the states translate the
  cursor to view space and forward hover / left click. GameOver gains a Back to
  leave without saving. Cursor visible again.
- `GameState` drives all gameplay input through the abstraction; `GameSettings`
  gets a `ControlSettings` (default keyboard bindings, not persisted — the
  rebinding UI + save/load land with the Options screen in v1.4.0).

---

## Released

### v1.0.4 — SFML DLL post-build copy & persistence

- Post-build step copies the config's four `sfml-*-3.dll` from `libs/SFML/bin/`
  next to the exe; `libs/SFML/` stays git-ignored.
- `utils/AppDataPath` (ported): saves → `%LOCALAPPDATA%\Alone Bull Company\
  Tessera\`. `utils/SafeFileWrite` (ported): atomic `.tmp` → `.bak` swap;
  `PreserveCorruptFile` keeps an unreadable file as `.corrupt`.
- `SettingsManager` / `HighScoreManager` write through `SafeFileWrite` and
  version each file; a wrong-version or unparseable file → `.corrupt` + defaults.
- Top-level `data/` gone; `Assets.h` `Data::Paths` → `SaveFile::`.

### v1.0.1 — Rename to Tessera, build hygiene & pause-menu crash fix

- Adopted the branch / PR / tag / release workflow; added `BACKLOG.md`,
  `.editorconfig`, `docs/`.
- Renamed `Tetris` → `Tessera` (GitHub repo, `.slnx` / `.vcxproj`, window title,
  main-menu title, README, `RootNamespace`).
- Dropped the broken Win32/x86 configurations; single x64 target on C++23.
- Warning level raised to `/W4`; fixed the discarded `[[nodiscard]]`
  render-texture results. (~40 pre-existing `/W4` warnings remain, cleared in
  v1.0.6.)
- `static_assert` the vendored SFML is exactly 3.1.0.
- Stopped tracking runtime player data; `data/.gitkeep` keeps the directory.
- Clamp the per-frame delta time (`Game::MaxFrameTime`).
- Replaced the `dynamic_cast<PauseState*>` render check with a `StateId` enum.
- Stop locking the tetromino twice on a hard drop.
- Screen-shake offset computed in `Update`, applied in `Render`; no
  divide-by-zero.
- Bounds-checked `Board::IntersectsLockedCells`.
- `StatisticsState` shares the context `HighScoreManager`.
- **Fixed the pause-menu crash (present since v1.0):** Pause > "Restart" /
  "Main Menu" destroyed the `PauseState` mid-method. `StateMachine` now queues
  every push / pop / clear / change and applies them only in
  `ApplyPendingChanges()`.

### v1.0.3 — Application rename, centralised events & MenuList

- `Game` → `Application`, moved to `src/app/`.
- Centralised window events: `Application::HandleInput()` /
  `ApplyWindowLifecycleEvent()`; `State::ProcessEvents(window)` →
  `State::HandleEvent(const sf::Event&)`.
- `State` base holds a `StateMachine&` + protected `RequestPush/Pop/Clear/
  Change`; copy/move deleted.
- `UI::Button::SetEnabled(bool)` + `disabledStyle` — removes GameOverState's
  Save-button hack.
- `UI::MenuList` — keyboard selection controller (wrap, skip disabled,
  `onSelectionChanged` / `onActivate`, `SelectAt()` stubbed). MainMenu / Pause /
  GameOver de-duplicated; move sound only fires on a real selection change.
- Mouse support cut (new feature) → v1.0.5 with the gamepad work.

### v1.0.2 — Board scan, row-clear phase & settings fixes

- Bounds-safety pass across `Board`; guard `LockTetromino` before indexing.
- Split `ClearFullRows` into `FindFullRows()` + `ClearRows(rows)` so a clear
  runs one scan, not two that could disagree.
- Row-clear is an explicit `Phase` (`Falling` / `ClearingRows`); the just-locked
  piece and its ghost stop drawing over the board during the clear animation.
- Next-piece preview centred on the piece's own bounding box.
- `AudioPlayer` caps concurrent voices at 32.
- `settings.txt` parsed into a scratch copy; defaults kept + file rewritten if
  any field is missing or out of range.
- FPS-limit slider reaches "Unlimited" at 0, steps by 10; not applied over vsync.

---

## Planned

### Phase 1b — port systems from ULA

**Phase 1 is complete** (v1.0.1 – v1.0.9). **v1.1.0 — Presentation** and
**v1.2.0 — Menu shell & Options** shipped (see Released). `ScreenFade`
transitions were dropped (the game is fast, transitions should feel instant).
The Options screen, `DisplayManager`, and the vibration / lightbar toggles
landed early in v1.2.0 (were pencilled for the old v1.5.0).

### Phase 2 — features

Version numbers below shifted +1 after v1.2.0 was re-scoped. **The v1.3.0
scope is still open — to be decided with the user.**

- **v1.3.0 — Modern rules.** Buffer rows above the visible field + guideline
  block-out / lock-out; SRS rotation + wall kicks + T-spin detection; lock
  delay + move reset; hold piece; 5-deep next queue; 7-bag confirmed as the
  randomiser; guideline scoring (back-to-back, combo, soft/hard-drop points,
  perfect clear); on-board callouts. Adds the Gameplay-settings toggles that
  depend on this: ghost piece, hold piece, next-queue length, 7-bag vs random
  (see Reminders).
- **v1.4.0 — Game modes.** Marathon, Sprint (40 lines), Ultra (2 minutes), Zen;
  per-mode records and extended stats. Wire real `StatisticsState` +
  `AchievementManager` in behind the disabled Records / Achievements ring
  entries.
- **v1.5.0 — Challenge ladder & achievements.** A ladder of short objective
  stages with modifiers (fixed sequences, garbage starts, invisible blocks,
  20G, big mode, "clear in N pieces"), escalating difficulty, per-stage stars;
  `AchievementManager` port.
- **v1.6.0 — Audio & HUD.** Dynamic-intensity music; a full gameplay SFX set;
  a nine-slice HUD redesign.
- **v1.7.0 — In-game menu overhaul.** Bring the Pause / Game Over screens up to
  the menu shell's visual standard (they still use the old `MenuScreenState`
  list style); reuse `MenuButtonColumn` / the shell transition.
- **Localization implementation** — the UI scaffold shipped in v1.2.0; the
  actual multi-language load + first-run picker + live switch is **deferred to
  the last development version**, once the rest of the game is done (retranslating
  churning strings mid-development is wasteful).
- **Visual overhaul (spans the versions above).** The whole game look is to be
  raised substantially. Part of that: replace `panel_background` /
  `button_background` / `game_background` etc. with art built to be
  nine-sliceable — the current frames aren't symmetric, so `NineSliceFrame`
  (added in v1.0.7) can't do much with them yet. New art unlocks the crisp
  frames and the nine-slice HUD redesign (v1.5.0).
- **v2.0.0 — Polish & docs.** Final polish, GitHub documentation, preview GIFs,
  release.

---

## Reminders

- **Gameplay-settings toggles to add in v1.3.0**, once the mechanic each needs
  exists: ghost piece, hold piece, next-queue length, 7-bag vs random. Extend
  `GameplayCategoryPanel` (rows + `GameSettings` fields, bump FormatVersion). If
  Gameplay grows past ~6 rows, split it into sub-sections.
- Localization language list is settled: English, Spanish, German, Russian,
  Ukrainian (UI scaffold in place; implementation deferred — see Planned).
- `SettingsRowList` / `MenuList` were replaced by `MenuButtonColumn` +
  `OptionRow` during v1.2.0 — the "fold into one selection model" TODO is done.

## Deferred ideas

- Garbage / attack model as groundwork for a future versus mode or AI opponent.
- Colorblind palettes and adjustable grid opacity.
- Replays / seed sharing for Sprint and Ultra.
