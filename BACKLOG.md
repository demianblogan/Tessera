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

## In progress

### v1.0.3 — Application rename, centralised events & MenuList

- **Renamed `Game` → `Application`** and moved it to `src/app/`, matching the
  Until Last Asteroid layout.
- **Centralised window events.** `Application::HandleInput()` polls once, routes
  `Closed` / `Resized` through `ApplyWindowLifecycleEvent()` and everything else
  to the active state. `State::ProcessEvents(sf::RenderWindow&)` becomes
  `State::HandleEvent(const sf::Event&)`; states no longer poll or each handle
  the close request.
- **`State` gained `RequestPush/Pop/Clear/Change`** (protected) so a state asks
  for a transition without reaching through `Context`.
- **`UI::Button::SetEnabled(bool)`** with a `disabledStyle`; a disabled button
  can't be selected. Removes GameOverState's hack of driving the Save button
  through `SetSelected(IsPlayerNameValid())`.
- **`UI::MenuList`** — a keyboard selection controller (wrap-around, skips
  disabled, "moved" / "activate" callbacks). MainMenu / Pause / GameOver drop
  their copy-pasted `selectedIndex` / `SelectPrevious` / `SelectNext` /
  `UpdateSelection` / `ActivateSelectedButton`. The move sound now plays only on
  an actual selection change. `SelectAt()` is stubbed for future mouse support.

Mouse support was cut from this version (it is a new feature; nothing before
v1.1.0) and will land with the gamepad work.

---

## Released

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

- **v1.0.4 — Persistence.** `AppDataPath`; `SafeFileWrite` atomic writes;
  settings format version + validation; move authored data to `assets/data/`;
  delete the top-level `data/`.
- **v1.0.5 — Input & GameState split.** `InputBinding` / `ActionMap` /
  `InputHandler` (Scancode + a gamepad-button variant); `DirectionalRepeater`
  for DAS/ARR; `GamepadManager` (ported whole, extended with discrete
  button→action mapping); mouse support wired into `MenuList::SelectAt`;
  rebinding UI. Split `GameState` into `TesseraGame` (rules) + `GameplayState` +
  `BoardRenderer` + `EffectsController`.
- **v1.0.6 — Visual pass.** `NeonGlow` real bloom; `NineSliceFrame` in
  `Button` / `Panel`; `TextLayout`; `ScreenFade`; `GlowingCursor`; per-effect
  toggles + screen-shake / reduce-motion; `/W4` → 0 + warnings-as-errors; first
  unit tests (`Board`, `Tetromino`, `TetrominoBag`, scoring).
- **v1.0.7 — Rumble & presentation.** `GamepadRumble` (XInput) + Vibration
  settings; `MenuBackground` parallax + `MenuIntroAnimation`; company splash +
  `GameVersion.h`.
- **v1.0.8 — Localization pipe & data skeleton.** `LocalizationManager` skeleton
  (all strings extracted, English wired) + `LocalizationRevision` + font system;
  threaded loading screen; data-driven content skeleton (piece defs, scoring,
  gravity as JSON).

### End of Phase 1

- Dedicated project-wide dead-code removal pass.

### Phase 2 — features

- **v1.1.0 — Modern rules.** Buffer rows above the visible field + guideline
  block-out / lock-out; SRS rotation + wall kicks + T-spin detection; lock
  delay + move reset; hold piece; 5-deep next queue; guideline scoring
  (back-to-back, combo, soft/hard-drop points, perfect clear); on-board
  callouts.
- **v1.2.0 — Game modes.** Marathon, Sprint (40 lines), Ultra (2 minutes), Zen;
  per-mode records and extended stats.
- **v1.3.0 — Challenge ladder & achievements.** A ladder of short
  objective stages with modifiers (fixed sequences, garbage starts, invisible
  blocks, 20G, big mode, "clear in N pieces"), escalating difficulty,
  per-stage stars; `AchievementManager` port.
- **v1.4.0 — Localization & options.** Full localization (**final language list
  decided here**); a complete Options screen (graphics / audio / controls /
  gameplay / language — **discuss the setting list before building**); a
  letterboxing `DisplayManager`.
- **v1.5.0 — Audio & HUD.** Dynamic-intensity music; a full SFX set; menu
  ambience; a nine-slice HUD redesign.
- **v1.6.0 — Main-menu overhaul.** Radical rework of the main menu — look,
  animation, structure. (A game-design change, deliberately after the code is
  mature.)
- **v2.0.0 — Polish & docs.** Final polish, GitHub documentation, preview GIFs,
  release.

---

## Reminders

- Before building the expanded Options screen (v1.4.0), have an explicit
  conversation about **which** settings to add and what goes in them.
- Localization language list is a **v1.4.0** decision.

## Deferred ideas

- Garbage / attack model as groundwork for a future versus mode or AI opponent.
- Colorblind palettes and adjustable grid opacity.
- Replays / seed sharing for Sprint and Ultra.
