# Tessera — Development Backlog

Source of truth for completed work, the version in progress, and planned
versions. Update it whenever an item changes scope or release.

Followed the *Until Last Asteroid* (ULA) process in two phases:

1. **Phase 1 — code maturity (`v1.0.x`).** No new gameplay. Turned the
   prototype-grade code into a professional, bug-free, de-duplicated, well
   layered codebase, porting proven systems from ULA. **Complete.**
2. **Phase 2 — features (`v1.1.0`+).** New systems and content on the
   cleaned-up foundation. Re-scoped on 2026-09-09 to a short wind-down — see
   **Planned**.

Each version: its own `v1.x.0-<slug>` branch (no prefix), a PR titled
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
  scoring table, gravity curve, escalation tiers, localization) as JSON under
  `assets/data/`. Runtime/user data goes to `%LOCALAPPDATA%`, never the repo.
  Core loop stays code.
- **Dead-code removal** — periodic project-wide passes, not piecemeal; the last
  one is part of v1.7.0. Ports are done whole, not pre-trimmed.

---

## Released

### v1.6.0 — Gameplay depth & escalation

Third version of the wound-down roadmap. The version that finishes the
gameplay: modern rules, the feedback toggles that go with them, and the
escalation that makes the one endless mode get harder over time.

- **Modern rules** — SRS rotation + wall kicks + T-spin (3-corner rule), lock
  delay + move reset, hold piece, 5-deep next queue, 7-bag, buffer rows above
  the field + block-out / lock-out, guideline-style scoring (Single / Double /
  Triple / Tetris multipliers, combo, back-to-back, Perfect Clear), on-board
  callouts. Authored piece shapes and SRS kick tables load from
  `assets/data/pieces.json` / `srs_kicks.json`, overriding the built-in tables
  if present.
- **Gameplay-settings toggles** with a mechanic behind them now: ghost piece,
  hold, next-queue length, 7-bag vs random, plus a Hold rebind key —
  `GameplayCategoryPanel` rows + `GameSettings` format 9.
- **Gamepad vibration on gameplay actions** — land, hard-drop, wall contact,
  row-clear, tetris, level-up, top-out, T-spin, gated by the existing
  Vibration toggle.
- **`gameplay/EscalationDirector`** — 4 cumulative tiers by elapsed time
  (Base, Speed Surge, Garbage, Chaos), each tier's first effect time-gated to
  a guaranteed window rather than left to play speed. In-game callouts,
  haptics, and golden-lock / garbage-row visuals react to the current tier.
- In-game controls legend swaps to Xbox / PlayStation icons when a pad is in
  use (`input/GamepadPrompts`).

### v1.5.0 — Gameplay presentation & Game Over

Second version of the wound-down roadmap. The in-game screen and the game-over
screen brought up to the menu-shell standard. No gameplay-rules changes.

- **In-game backdrop** — `game_background.png` (renamed `menu_background.png`,
  now only the main menu uses it) replaced by `gameplay_background.jpg`. New
  `rendering/SceneMotion`: a springy positional offset the player drives —
  moves, rotations, soft / hard drops, landings, line clears (double for a
  tetris) and level-ups each add an impulse, a damped spring pulls it back, and
  a slow idle drift plays underneath. `GameplayState` draws the backdrop 7 %
  oversized and centred, shifted by that offset, so it parallaxes behind the
  static board and HUD. *(A depth-layered ambient particle field was built and
  cut — every rendition either fought the pixel look or was too faint to earn
  its place.)*
- **HUD rebuilt** (`rendering/GameplayHud`) — the generic
  `UI::Layout`/`Panel`/`Label` HUD is gone. Square nine-slice frames
  (`menu_background_blue_frame`) hug the board's outer wall: HOLD and LEVEL
  down the left, NEXT / SCORE / LINES / TIME down the right. Clearing rows pops
  SCORE and LINES, a level-up pops LEVEL (cyan outline glow + a scale-and-tint
  on the value). `GameplaySession` exposes `GetElapsedSeconds()`;
  `utils/TimeFormat` formats the TIME readout. The left-hand controls text
  block is replaced by an always-on legend panel under the left column —
  Move / Rotate / Soft Drop / Hard Drop / Pause with live, layout-independent
  key names. `ui/Panel` / `ui/Layout` and `TextureID::PanelBackground` are now
  unused by the game (still built).
- **Options → HUD category** (`states/HudCategoryPanel`, watermelon accent,
  `menu_background_white_red_frame`) — one on/off toggle per panel (Hold, Next,
  Score, Lines, Level, Time) plus the controls legend, all default on, so a
  player can strip the screen to just the well. `GameSettings` format 7 adds
  the seven `hud*` flags; `GameplayState` reads them in its ctor.
- **Game Over rebuilt** (`states/GameOverState`, no longer on
  `MenuScreenState`) — one screen, two states. A framed panel over the dimmed,
  crumbled board (new death beat in `GameplayState` / `BoardRenderer`:
  `deathProgress` greys and drops the locked cells, then the screen darkens
  into the game-over dim with no cut) shows the run summary — score, lines,
  level, time — with a dying-neon flicker and a rare chromatic glitch on the
  "GAME OVER" heading. **New record:** the frame goes gold, the heading's neon
  goes gold, firework shells rise and burst behind the panel and four jets of
  sparks stream out of its corners (`ui/Celebration`, CPU particles). A
  recessed name field (grey "Enter your name" prompt, clears on input, 14-char
  cap) with a **Save Record** button to its right — dim until the field has
  content, saves on click or Enter, then settles into a disabled "Saved".
  Leaving with an unsaved name raises a confirm dialog. Play Again / Main Menu
  below; Save Record joins the Up/Down focus ring while it is actionable.
- **`ui/ConfirmDialog`** gains mouse control (`PointerMoved` / `PointerPressed`)
  and plays one press sound instead of the caller adding a second;
  `states/OptionsSfx::DialogPick` (now unused) removed.
- **Cleanup** — `states/MenuScreenState` and `ui/MenuList` deleted
  (`GameOverState` was the last consumer); dead `ScreenHost::ExitTo` removed;
  the old widget-tree leaves `ui/Button` / `ui/Label` / `ui/Layout` / `ui/Panel`
  / `ui/Spacer` deleted (the HUD rewrite orphaned the last of them) along with
  the `button_background.png` / `panel_background.png` textures; `ui/CarouselMenu`
  centres each entry on its visible ink so the nav arrows sit symmetrically.
  *(`ui/Element` and `ui/Slider` were kept at the time on the assumption the
  Audio panel's volume sliders used them; they actually use `UI::SliderRow`
  (`ui/OptionRow`, shipped in v1.2.0) and `ui/Element` / `ui/Slider` had no
  callers left — found and removed in the v1.7.0 sweep.)*

### v1.4.0 — Menu cleanup & flow

First version of the wound-down roadmap (see the note under Planned). No
gameplay-rules changes; the front-end made consistent and finished.

- **Mode select removed.** The "Start Game" ring entry is now **"Play"** and
  drops straight into gameplay; `states/ModeSelectScreen` and its
  `TextKey::ModeSelect` keys are gone. The disabled "Achievements" ring entry is
  removed too. Every remaining ring entry pins its own colour (the 5-entry ring
  otherwise handed Quit the same red as Options).
- **`states/PlayTransition`** — activating "Play" freezes the whole menu frame
  into a snapshot, runs it through `mosaic.frag` so it solidifies top-down, then
  slides the solid sheet down and off, revealing the gameplay behind it.
  `GameplayState` gains a short `playIntro` hold (session + input frozen) so the
  first piece doesn't fall mid-transition. Pause → Back to Main Menu fades to
  black; `MenuShell` fades up from black on entry, so every route into the menu
  arrives on a fade.
- **Per-menu frames.** Credits, the Options category panels and the Gamepad
  reference nine-slice a frame in their own accent hue — cyan / blue / green /
  purple / brown — via new `TextureID::UiFrame*`; the shared source-border width
  is `UI::MenuFrameSourceBorder`. The old `frame.png` is gone.
- **`ui/ConfirmDialog` rebuilt** to the menu standard: a gold nine-slice box
  that slides in from the top and back out before the choice takes effect, two
  `MenuLabel` buttons (Yes green / No red) with the idle wave, selection glow,
  press flash and nav sound the rest of the menus have.
- **Credits** — new developer blurb and link list (email, LinkedIn, Instagram,
  repo, portfolio, two YouTube channels), a wider panel and larger text.
- **`states/RecordsScreen`** replaces the legacy `StatisticsState`: a
  `MenuScreen` on the shared `ScreenHost`, reached through the header-morph
  transition. A purple nine-slice panel holds a ten-row leaderboard in aligned
  columns (rank / name / score / lines / level; empty rows show a dash), with
  **Reset** (confirm dialog → clear + save) and **Back** below it. Records now
  keep **lines cleared** and **level reached** alongside the name and score
  (`HighScoreManager` format 2, top 10 was top 5); `GameplaySession` exposes
  `GetLinesCleared()`. The `menu_background.png` texture, used only by the old
  screen, is removed.
- "Back to Main Menu" shortened to "Back" on Records, Credits and Options.

### v1.0

- Original portfolio version: playable Tetris with 7-bag pieces, ghost piece,
  progressive difficulty, next-piece preview, screen shake, row-clear and
  landing effects, a custom UI framework, a settings menu, top-5 high scores,
  and shader-based CRT / blur / glow post-processing.

### v1.3.0 — In-game menu & mode select

A new Pause screen built to the menu-shell standard, and a mode-select screen
behind "Start Game". No gameplay-rules changes; Game Over untouched (its rework
is v1.5.0).

- **`states/ScreenHost`** — the screen swap + `ui/MenuHeader` + forward / back
  transition + DualSense-lightbar handoff, extracted from `MenuShell` (now a
  thin subclass supplying the animated menu background and the `MainMenuScreen`
  home). Sub-screens take a `ScreenHost&`; the ring's backdrop shove goes
  through `ScreenHost::OnNavigate`. A `HomeDrivesHeader()` / `OnHomeRebuilt()`
  hook lets a host keep a persistent header on its home screen.
- **Pause** (`states/PauseState : ScreenHost`, `states/PauseMenuScreen`) — on
  pause, `GameplayState` snapshots the frame into a `RenderTexture`;
  `assets/shaders/mosaic.frag` "solidifies" it (blur + large-pixel quantise +
  darken) with a front that sweeps top-down on entry and bottom-up on Resume.
  A "PAUSE" header (amber, the mode-select "Campaign" hue) rises from the top
  edge as a left `ui/MenuButtonColumn` slides in low in the frame: Resume /
  Restart / Options / Back to Main Menu, each in its own colour. Resume
  reverses the whole animation before popping. Restart and Back to Main Menu
  raise a `ui/ConfirmDialog`. Options opens over the frozen frame through the
  shared `ScreenHost` transition; settings apply live.
- **Mode select** (`states/ModeSelectScreen`) — "Start Game" now morphs into a
  two-level column like Options → Controls: **Campaign** (Start New Campaign /
  Continue Campaign / Select Level) and **Other Modes** (Marathon / Sprint /
  Ultra / Zen / Player vs Player). Only Marathon is live; the rest are disabled
  stubs. *(The stubs and the concept doc `docs/CAMPAIGN_AND_MODES.md` were
  removed in v1.4.0 when the campaign / modes roadmap was dropped — see Planned.)*
- **Gameplay music removed** — the old track did not fit; `MusicID::Gameplay`
  and its asset are gone. Gameplay stays music-free (a dynamic score is not
  planned under the wound-down roadmap).
- **Cleanup** — `Lerp` / `SmoothStep` / `EaseOut*` deduplicated into
  `ui/Easing.h` (were copy-pasted in ~10 TUs); three unused fonts
  (`BarNewRomanPix*`, `EpilepsySans`) deleted; dead `ScreenHost::ShowScreen` /
  `IsTransitioning` removed.

### v1.2.0 — Menu shell & Options

Originally scoped as "Modern rules"; **re-scoped** to the menu shell and a
complete Options screen. No gameplay-rules changes.

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

**Roadmap reset — 2026-09-09.** The campaign / multi-mode / local-PvP / AI
roadmap is dropped. Tessera is being wound down as a small, cleanly-built game:
**one endless mode that escalates over time**, with the menu system as its
showpiece — finished, polished, then done. No campaign, no level select, no
save-progress system, no achievements, no PvP, no AI. Four short versions, each
verified before the next, easy work before hard.

Why: the core game holds no interest for the author (a shooter and a platformer
did) and forcing months of Tetris content is the wrong trade. The menu framework
was always meant as a reusable cross-project UI library — it carries forward
regardless.

**v1.4.0 — Menu cleanup & flow**, **v1.5.0 — Gameplay presentation & Game
Over** and **v1.6.0 — Gameplay depth & escalation** shipped (see Released).

### v1.7.0 — Localization & final refactor

The final version. After it the game is done — there is no v2.0.

- **Localization** — ✅ done. Language enum + `GameSettings` persistence
  (format 10), catalog loading with an English fallback merge, a revision
  counter so every open screen switches live with no restart, a first-run
  picker before the main menu, and full translations (English, Spanish,
  German, Russian, Ukrainian) for every catalog key including the on-board
  callouts. The UI scaffold shipped in v1.2.0.
- **Gameplay visual effects** — ✅ done. Line-clear effects scale with rank
  (Single through Tetris), a real bloom glow on the well border while a combo
  holds, escalation tiers (see `EscalationDirector`) get their own ambience
  and HUD reaction, a soft trail between the falling piece and its ghost, an
  animated hard-drop slide instead of an instant teleport, and grey impact
  dust on both hard-drop landings and wall hits.
- **Audio pass** — partly done. `audio/MusicPlayer` shuffles three gameplay
  tracks into an endless, non-repeating playlist and ducks them (same easing
  as the pause menu) while the game-over sting plays; every menu nav / toggle
  sound is now consistently pitched by direction (vertical lists, checkboxes,
  mouse-clicked slider arrows, the gamepad reference panel). Still open: a
  number of gameplay sounds are missing or reuse a placeholder (e.g. Hold
  still borrows the rotate sound — see `GameplayState::TryHold`).
- **A shortcut to the game** — resolved as just the exe's own icon
  (`assets/other/icon.ico` via `Tessera.rc`), not a desktop/Start Menu
  shortcut or installer.
- **Project-wide refactor and polish** — tighten the feel, clean the code, pull
  back any improvements from ULA's shared helpers (`NineSliceFrame`,
  `TextLayout`, `NeonGlow`, `GamepadHaptics`), a final dead-code sweep.
  ✅ The blurred-backdrop render path was removed earlier
  (`State::Backdrop::BlurredPrevious` was returned by no state once Pause
  moved to `mosaic.frag`) — `blur.frag` / `ShaderID::Blur`,
  `Application::gameplayTexture` / `finalTexture`, and
  `StateMachine::RenderStatesExceptTop` / `RenderTopState` are all gone.
  ✅ The broader pass landed too: the old `ui/Element` + `ui/Slider` widgets
  (superseded by `UI::SliderRow` since v1.2.0) deleted; three orphaned
  localization keys (`options.coming_soon`, `options.key_pause`,
  `hud.controls`) removed from `TextKeys.h` and every catalog; the near-
  identical toggle/carousel row-sound branch in `GameplayCategoryPanel` /
  `GraphicsCategoryPanel` / `HudCategoryPanel` collapsed into shared
  `SettingsCategoryPanel::*RowByType` helpers; `pieces.json` / `srs_kicks.json`
  / `audio_balance.json` / `haptics.json` moved from hardcoded strings in
  `Application.cpp` into `Assets::Paths::Data` alongside every other asset
  path; a couple of comments that had gone stale (the HUD's hold-piece
  outline, the Hold sound) reworded to match what the code actually does.
- **Release** — README as a finished piece, screenshots / GIFs, itch.io page,
  `Tessera-v1.7.0-win64.zip`, GitHub Release as the last one.

### Visual overhaul (spans v1.6.0–v1.7.0)

The whole game look is still to be raised. `panel_background` /
`button_background` are gone (removed in v1.5.0 with `ui/Button` / `ui/Panel`,
their last users); any further legacy art wants replacing or removing as the
author sources new art for each version.

---

## Deferred ideas

- Colorblind palettes and adjustable grid opacity — candidates for the v1.7.0
  polish pass.
- Sprint (40 lines) / Ultra (2 minutes) as extra record modes — cheap to add on
  top of the v1.6.0 escalation work if wanted; not currently planned.
